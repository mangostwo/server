message("===================================================")
message("Build type            : ${CMAKE_BUILD_TYPE}")
message("Revision              : ${MANGOS_HASH}")
message("Install server(s) to  : ${BIN_DIR}")
message("Install configs to    : ${CONF_INSTALL_DIR}")

if(BUILD_MANGOSD)
    message("Build main server     : Yes (default)")
    if(SCRIPT_LIB_ELUNA)
        message("+-- with Eluna script engine")
    endif()
    if(SCRIPT_LIB_SD3)
        message("+-- with SD3 script engine")
    endif()
    if(PLAYERBOTS)
        message("+-- with PlayerBots")
    endif()
else()
    message("Build main server     : No")
endif()

if(BUILD_REALMD)
    message("Build login server    : Yes (default)")
else()
    message("Build login server    : No")
endif()

if(SOAP)
    message("Support for SOAP      : Yes")
else()
    message("Support for SOAP      : No (default)")
endif()

if(BUILD_TOOLS)
    message("Build tools           : Yes (default)")
else()
    message("Build tools           : No")
endif()
message("")
message("===================================================")
