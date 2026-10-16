cmake_minimum_required(VERSION 3.16)
project(miryu-lang
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "A simple CLI to set the system locale on Linux/WSL"
    HOMEPAGE_URL "https://github.com/miryu/miryu-lang"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- gettext / i18n ---------------------------------------------------------
find_package(Intl)
find_package(Gettext)

if(Intl_FOUND)
    set(HAVE_GETTEXT 1)
    include_directories(${Intl_INCLUDE_DIRS})
endif()

include(GNUInstallDirs)

# Localedir where the compiled .mo files will be installed.
if(NOT DEFINED LOCALEDIR)
    set(LOCALEDIR "${CMAKE_INSTALL_FULL_LOCALEDIR}" CACHE PATH
        "Locale data directory")
endif()

# --- executable -------------------------------------------------------------
add_executable(miryu-lang src/main.cpp)

if(Intl_FOUND)
    target_compile_definitions(miryu-lang PRIVATE HAVE_GETTEXT=1)
    target_link_libraries(miryu-lang PRIVATE ${Intl_LIBRARIES})
endif()

target_compile_definitions(miryu-lang PRIVATE
    LOCALEDIR="${LOCALEDIR}"
)

install(TARGETS miryu-lang
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# --- translations -----------------------------------------------------------
if(Gettext_FOUND)
    set(POT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/po/miryu-lang.pot)
    set(PO_LANGUAGES zh_CN zh_TW ja de fr ko)

    # Update .pot from source.
    add_custom_target(update-pot
        COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}
            --from-code=UTF-8
            --package-name=${PROJECT_NAME}
            --package-version=${PROJECT_VERSION}
            --keyword=_
            --keyword=N_
            --output=${POT_FILE}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
        COMMAND ${CMAKE_COMMAND} -D POT_FILE=${POT_FILE} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FixPotCharset.cmake
        COMMENT "Updating translation template (pot)"
    )

    set(MO_FILES)
    foreach(lang ${PO_LANGUAGES})
        set(PO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/po/${lang}.po)
        set(MO_FILE ${CMAKE_CURRENT_BINARY_DIR}/locale/${lang}/LC_MESSAGES/miryu-lang.mo)
        list(APPEND MO_FILES ${MO_FILE})

        add_custom_command(
            OUTPUT  ${MO_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    ${CMAKE_CURRENT_BINARY_DIR}/locale/${lang}/LC_MESSAGES
            COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
                    -o ${MO_FILE} ${PO_FILE}
            DEPENDS ${PO_FILE}
            COMMENT "Compiling translation for ${lang}"
        )

        install(FILES ${MO_FILE}
            DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${lang}/LC_MESSAGES
        )
    endforeach()

    add_custom_target(translations ALL DEPENDS ${MO_FILES})
    add_dependencies(miryu-lang translations)
endif()

# --- packaging --------------------------------------------------------------
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_NAME "miryu-lang")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "miryu@example.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple CLI to set the system locale on Linux/WSL")
include(CPack)
