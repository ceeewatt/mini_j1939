.PHONY: all configure clean

all:
	cmake --build build --target node1 node2

configure:
	mkdir -p build
	cmake -S . -B build/ -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -DJ1939_DEMO=1 -DJ1939_DISABLE_ADDRESS_CLAIM=1 -DJ1939_DISABLE_TRANSPORT_PROTOCOL=1

clean:
	rm -rf build/*
