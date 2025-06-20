find_package(Catch2 3 REQUIRED)

set(MINI_J1939_TEST mini_j1939_test)

set(SOURCES
    test_j1939.cpp
    test_j1939.hpp
    test_j1939_private.cpp
    test_j1939_address_claim.cpp
    test_j1939_transport_protocol.cpp
)

add_executable(${MINI_J1939_TEST}
    ${SOURCES}
)

target_link_libraries(${MINI_J1939_TEST} PRIVATE
    Catch2::Catch2WithMain
    MiniJ1939::mini_j1939_lib
)
