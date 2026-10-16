// miryu-lang - a simple CLI to set the system locale on Linux/WSL
// Copyright (C) 2026  Miryu Project
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <string>
#include <map>
#include <unistd.h>

#ifdef HAVE_GETTEXT
#include <libintl.h>
#define _(String) gettext(String)
#else
#define _(String) (String)
#endif

// N_() is a no-op marker recognised by xgettext so that strings stored in
// tables (where gettext() cannot be called at initialisation time) are still
// extracted into the translation catalogue.
#ifndef N_
#define N_(String) (String)
#endif

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

#define PACKAGE "miryu-lang"

// A small alias table for common / ambiguous language codes.  Entries here
// provide a friendly display name and an explicit locale.  Any code not listed
// here is handled by the generic to_locale() heuristic below, so the program
// supports every language whose locale follows the standard xx_XX.UTF-8
// naming convention (ru_RU, cs_CZ, it_IT, es_ES, pl_PL, ...).
struct LangInfo {
    const char* locale;
    const char* display_name;
};

static const std::map<std::string, LangInfo> lang_table = {
    { "zh-cn", { "zh_CN.UTF-8", N_("Simplified Chinese")  } },
    { "zh-tw", { "zh_TW.UTF-8", N_("Traditional Chinese") } },
    { "zh-hk", { "zh_HK.UTF-8", N_("Chinese (Hong Kong)") } },
    { "en-us", { "en_US.UTF-8", N_("English (US)")        } },
    { "en-gb", { "en_GB.UTF-8", N_("English (UK)")        } },
    { "pt-br", { "pt_BR.UTF-8", N_("Portuguese (Brazil)") } },
    { "pt-pt", { "pt_PT.UTF-8", N_("Portuguese (Portugal)") } },
    { "es-es", { "es_ES.UTF-8", N_("Spanish")             } },
    { "es-mx", { "es_MX.UTF-8", N_("Spanish (Mexico)")    } },
    { "it",    { "it_IT.UTF-8", N_("Italian")             } },
    { "ja",    { "ja_JP.UTF-8", N_("Japanese")            } },
    { "de",    { "de_DE.UTF-8", N_("German")              } },
    { "fr",    { "fr_FR.UTF-8", N_("French")              } },
    { "ko",    { "ko_KR.UTF-8", N_("Korean")              } },
    { "ru",    { "ru_RU.UTF-8", N_("Russian")             } },
    { "cs",    { "cs_CZ.UTF-8", N_("Czech")               } },
    { "pl",    { "pl_PL.UTF-8", N_("Polish")              } },
    { "nl",    { "nl_NL.UTF-8", N_("Dutch")               } },
    { "sv",    { "sv_SE.UTF-8", N_("Swedish")             } },
    { "da",    { "da_DK.UTF-8", N_("Danish")              } },
    { "fi",    { "fi_FI.UTF-8", N_("Finnish")             } },
    { "no",    { "no_NO.UTF-8", N_("Norwegian")           } },
    { "tr",    { "tr_TR.UTF-8", N_("Turkish")             } },
    { "ar",    { "ar_SA.UTF-8", N_("Arabic")              } },
    { "he",    { "he_IL.UTF-8", N_("Hebrew")              } },
    { "th",    { "th_TH.UTF-8", N_("Thai")                } },
    { "vi",    { "vi_VN.UTF-8", N_("Vietnamese")          } },
    { "id",    { "id_ID.UTF-8", N_("Indonesian")          } },
    { "uk",    { "uk_UA.UTF-8", N_("Ukrainian")           } },
    { "hu",    { "hu_HU.UTF-8", N_("Hungarian")           } },
    // Codes whose region is not the uppercased language code:
    { "nb",    { "nb_NO.UTF-8", N_("Norwegian Bokmal")    } },
    { "nn",    { "nn_NO.UTF-8", N_("Norwegian Nynorsk")   } },
    { "sr",    { "sr_RS.UTF-8", N_("Serbian")             } },
    { "yue",   { "yue_HK.UTF-8", N_("Cantonese")          } },
    { "nan",   { "nan_TW.UTF-8", N_("Hokkien")            } },
    { "cmn",   { "cmn_TW.UTF-8", N_("Mandarin Chinese")   } },
    { "hak",   { "hak_TW.UTF-8", N_("Hakka")              } },
    { "lzh",   { "lzh_TW.UTF-8", N_("Classical Chinese")  } },
    { "eo",    { "eo.UTF-8",     N_("Esperanto")           } },
};

// Convert a user-provided language identifier into a locale name.
//
// Accepted forms (anything listed in `locale -a` is supported):
//   "ru"             -> "ru_RU.UTF-8"        (two-letter code -> xx_XX.UTF-8)
//   "zh-cn"          -> "zh_CN.UTF-8"        (dash normalised to underscore)
//   "en_US"          -> "en_US.UTF-8"        (region present, append codeset)
//   "fr_FR.UTF-8"    -> "fr_FR.UTF-8"        (already a full locale, used as-is)
//   "ru_RU.utf8"     -> "ru_RU.UTF-8"        (lowercase utf8 normalised)
//   "be_BY@latin"    -> "be_BY.UTF-8@latin"  (@modifier preserved after codeset)
//   "ca_ES.utf8@valencia" -> "ca_ES.UTF-8@valencia"
//   "eo" / "tok"     -> "eo.UTF-8"           (codes without a region)
//
// Locale names are case-sensitive: the language code is lowercase and the
// territory/region code is uppercase.  We therefore normalise the case of both
// parts so that e.g. "ja-jp" -> "ja_JP.UTF-8", not "ja_jp.UTF-8".
static std::string to_locale(const std::string& input)
{
    std::string s = input;

    // Normalise dashes to underscores (e.g. "pt-br" -> "pt_br").
    for (auto& c : s) {
        if (c == '-') c = '_';
    }

    // Split off any @modifier (e.g. "be_BY@latin").  The codeset must be
    // inserted before the modifier.
    std::string modifier;
    auto at_pos = s.find('@');
    if (at_pos != std::string::npos) {
        modifier = s.substr(at_pos);
        s = s.substr(0, at_pos);
    }

    // Split off an existing codeset (e.g. "fr_FR.UTF-8", "ru_RU.utf8").
    std::string codeset;
    auto dot_pos = s.find('.');
    if (dot_pos != std::string::npos) {
        codeset = s.substr(dot_pos + 1);
        s = s.substr(0, dot_pos);
    }

    // Now `s` is either a bare language code ("ru", "yue") or a
    // "language_region" pair ("en_US", "ja_jp").  Normalise the case:
    // the language part is lowercase, the region part (if present) uppercase.
    std::string language = s;
    std::string region;
    auto us_pos = s.find('_');
    if (us_pos != std::string::npos) {
        language = s.substr(0, us_pos);
        region   = s.substr(us_pos + 1);
        for (auto& c : language) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        for (auto& c : region)   c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        s = language + "_" + region;
    } else {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // Re-attach / inject the codeset, normalising lowercase "utf8" -> "UTF-8".
    if (!codeset.empty()) {
        if (codeset == "utf8" || codeset == "UTF8" || codeset == "utf-8") {
            codeset = "UTF-8";
        }
        return s + "." + codeset + modifier;
    }

    // No codeset was supplied.  If a region is present just append .UTF-8.
    if (!region.empty()) {
        return s + ".UTF-8" + modifier;
    }

    // Bare language code.  For two-letter codes the canonical region is the
    // uppercased language code (ru -> ru_RU, cs -> cs_CZ, ...).  This matches
    // the vast majority of single-country languages.
    if (s.size() == 2) {
        std::string up = s;
        for (auto& c : up) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s + "_" + up + ".UTF-8" + modifier;
    }

    // Fallback for longer codes (e.g. three-letter ISO 639-3 codes such as
    // "yue", "nan", "tok", "eo"): just append the UTF-8 codeset.
    return s + ".UTF-8" + modifier;
}

static void print_help(void)
{
    std::printf("%s\n", _("Usage: miryu-lang <language-code>"));
    std::printf("\n");
    // TRANSLATORS: keep "zh-cn", "en-us", "ru", "cs", "ja", "de", "fr", "ko"
    // as literal examples.
    std::printf("%s\n", _("Examples: miryu-lang zh-cn | miryu-lang en-us | miryu-lang ru | miryu-lang cs"));
    std::printf("\n");
    std::printf("%s\n", _("Any language code is accepted; the locale is derived automatically."));
}

static void print_common_shortcuts(void)
{
    std::printf("%s\n", _("Common shortcuts:"));
    for (const auto& kv : lang_table) {
        std::printf("  %-8s -> %s  (%s)\n", kv.first.c_str(),
                    _(kv.second.display_name), kv.second.locale);
    }
}

static void print_invalid_args(void)
{
    std::fprintf(stderr, "%s\n",
                 _("Invalid command-line arguments. Type miryu-lang --help for help."));
}

static void print_not_root(void)
{
    std::fprintf(stderr, "%s\n", _("Root privileges are required to change the language."));
}

int main(int argc, char* argv[])
{
#ifdef HAVE_GETTEXT
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);
#endif

    // --help / -h can be checked before the root check so that any user can
    // read the usage information.
    if (argc == 2) {
        if (std::strcmp(argv[1], "--help") == 0 ||
            std::strcmp(argv[1], "-h") == 0) {
            print_help();
            print_common_shortcuts();
            return 0;
        }
    }

    // Root privilege is required to change the system locale.
    if (geteuid() != 0) {
        print_not_root();
        return 1;
    }

    if (argc != 2) {
        print_invalid_args();
        return 1;
    }

    std::string arg = argv[1];

    // Resolve the locale: prefer the explicit alias table, otherwise derive
    // it generically so that every language code is supported.
    std::string locale_name;
    const char* display = nullptr;
    auto it = lang_table.find(arg);
    if (it != lang_table.end()) {
        locale_name = it->second.locale;
        display     = it->second.display_name;
    } else {
        locale_name = to_locale(arg);
        display     = locale_name.c_str();
    }

    // Build and execute the localectl command.
    std::string cmd = "localectl set-locale LANG=";
    cmd += locale_name;

    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::fprintf(stderr, "%s\n", _("Failed to change language. Please check if the system supports the localectl command and the locale is available."));
        return 1;
    }

    // TRANSLATORS: %s will be replaced by the language name or locale.
    std::printf(_("Changed to %s. Restart WSL to apply the changes.\n"),
                display ? _(display) : locale_name.c_str());
    return 0;
}
