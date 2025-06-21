TARGETS=all
J1939_NODES=1
BUILD_DIR=build
CMAKE_BUILD_TYPE=Debug
J1939_DEMO=ON
UNIT_TEST=OFF
LISTENER_ONLY_MODE=OFF

.PHONY: all configure clean

all:
	cmake --build ${BUILD_DIR} --target ${TARGETS}

configure:
	mkdir -p ${BUILD_DIR}
	cmake -S . -B ${BUILD_DIR} \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
		-DJ1939_NODES=${J1939_NODES} \
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
		-DJ1939_DEMO=${J1939_DEMO} \
		-DJ1939_LISTENER_ONLY_MODE=${LISTENER_ONLY_MODE} \
		-DBUILD_TESTING=${UNIT_TEST}
	if [ ! -L compile_commands.json ]; then ln -s ${BUILD_DIR}/compile_commands.json .; fi

clean:
	rm -rf ${BUILD_DIR}/*
