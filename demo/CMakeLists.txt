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

target_link_libraries(node1 PRIVATE MiniJ1939::mini_j1939_lib)
target_link_libraries(node2 PRIVATE MiniJ1939::mini_j1939_lib)
