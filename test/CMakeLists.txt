find_package(Catch2 3 REQUIRED)

add_executable(test_j1939
    test_j1939.cpp
    ${MINI_J1939_SRCS}
)

target_include_directories(test_j1939 PRIVATE ${MINI_J1939_INCLUDE_DIRS})
target_compile_definitions(test_j1939 PRIVATE J1939_UNIT_TEST)
target_link_libraries(test_j1939 PRIVATE Catch2::Catch2WithMain)
