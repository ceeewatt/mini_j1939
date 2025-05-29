cmake_minimum_required(VERSION 3.28)
project(mini_j1939)

option(J1939_DEMO "Build demo project" OFF)
option(J1939_UNIT_TEST "Build unit tests" OFF)
option(J1939_DISABLE_ADDRESS_CLAIM "Disable the address claim procedure" OFF)
option(J1939_DISABLE_TRANSPORT_PROTOCOL "Disable the transport protocol procedure for sending messages with more than 8 bytes of payload" OFF)

set(MINI_J1939_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/j1939.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/j1939.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/j1939_transport_protocol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/j1939_address_claim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/j1939_define.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/j1939_private.h
)

set (MINI_J1939_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(mini_j1939 STATIC
    ${MINI_J1939_SRCS}
)
target_link_libraries(mini_j1939 PRIVATE m)

if (${J1939_DEMO})
    message(STATUS "[J1939] Enabling demo project")
    add_subdirectory(demo)
endif()

if (${J1939_UNIT_TEST})
    message(STATUS "[J1939] Enabling unit tests")
    add_subdirectory(test)
endif()

if (${J1939_DISABLE_ADDRESS_CLAIM})
    message(STATUS "[J1939] Disabling address claim")
    list(REMOVE_ITEM MINI_J1939_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/j1939_address_claim.c)
    target_compile_definitions(mini_j1939 PRIVATE J1939_DISABLE_ADDRESS_CLAIM)
endif()

if (${J1939_DISABLE_TRANSPORT_PROTOCOL})
    message(STATUS "[J1939] Disabling transport protocol")
    list(REMOVE_ITEM MINI_J1939_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/j1939_transport_protocol.c)
    target_compile_definitions(mini_j1939 PRIVATE J1939_DISABLE_TRANSPORT_PROTOCOL)
endif()
