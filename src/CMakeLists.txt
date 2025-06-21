set(MINI_J1939_LIB mini_j1939_lib)

set(SOURCES
    j1939.h
    j1939_define.h
    j1939_private.c
    j1939_private.h
    j1939_address_claim.c
    j1939_address_claim.h
    j1939_transport_protocol.h
    j1939_transport_protocol.c
    j1939_transport_protocol_helper.c
    j1939_transport_protocol_helper.h
)

if (NOT J1939_NODES)
    message(FATAL_ERROR "[mini_j1939] Set J1939_NODES to the number of Controller Applications used.")
endif()

add_library(${MINI_J1939_LIB} STATIC
    ${SOURCES}
)

# Make this target visible to other subdirectories through the alias name
add_library(MiniJ1939::mini_j1939_lib ALIAS ${MINI_J1939_LIB})

target_include_directories(${MINI_J1939_LIB} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(${MINI_J1939_LIB} PRIVATE
    J1939_NODES=${J1939_NODES}
)
target_compile_options(${MINI_J1939_LIB} PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Wfatal-errors
)

if (J1939_LISTENER_ONLY_MODE)
    target_compile_definitions(${MINI_J1939_LIB} PRIVATE J1939_LISTENER_ONLY_MODE)
endif()

if (BUILD_TESTING)
    target_compile_definitions(${MINI_J1939_LIB} PRIVATE UNIT_TEST)
endif()
