if ("${CLIENT_BUILD_TYPE}" STREQUAL "Classic")
    set(MANGOS_EXP "CLASSIC")
    set(MANGOS_PKG "Mangos Zero")
endif ()

if ("${CLIENT_BUILD_TYPE}" STREQUAL "TBC")
    set(MANGOS_EXP "TBC")
    set(MANGOS_PKG "Mangos One")

endif ()

if ("${CLIENT_BUILD_TYPE}" STREQUAL "WotLK")
    set(MANGOS_EXP "WOTLK")
    set(MANGOS_PKG "Mangos Two")
endif ()

if ("${CLIENT_BUILD_TYPE}" STREQUAL "Cata")
    set(MANGOS_EXP "CATA")
    set(MANGOS_PKG "Mangos Three")
endif ()

if ("${CLIENT_BUILD_TYPE}" STREQUAL "Mop")
    set(MANGOS_EXP "MISTS")
    set(MANGOS_PKG "Mangos Four")
endif ()

set(MANGOS_WORLD_VER 2021010100)
set(MANGOS_REALM_VER 2021010100)
set(MANGOS_AHBOT_VER 2021010100)
