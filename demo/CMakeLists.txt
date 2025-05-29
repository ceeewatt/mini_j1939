add_executable(node1
    node1.c
    j1939_app.c
    j1939_app.h
    node_helper.c
    node_helper.h
)
add_executable(node2
    node2.c
    j1939_app.c
    j1939_app.h
    node_helper.c
    node_helper.h
)

target_include_directories(node1 PRIVATE ${MINI_J1939_INCLUDE_DIRS})
target_include_directories(node2 PRIVATE ${MINI_J1939_INCLUDE_DIRS})

target_link_libraries(node1 PRIVATE mini_j1939)
target_link_libraries(node2 PRIVATE mini_j1939)
