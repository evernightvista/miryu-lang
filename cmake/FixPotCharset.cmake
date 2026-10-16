# FixPotCharset.cmake - replace the xgettext CHARSET placeholder with UTF-8.
# xgettext always emits "charset=CHARSET" in the generated .pot header; this
# script normalises it to UTF-8 which is the encoding used by the source.
file(READ "${POT_FILE}" content)
string(REPLACE "charset=CHARSET" "charset=UTF-8" content "${content}")
file(WRITE "${POT_FILE}" "${content}")
