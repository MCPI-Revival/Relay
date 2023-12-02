.DEFAULT_GOAL := build

BUILD_DIR ?= build/
SOURCE_DIR ?= MCPIRelay/

.PHONY: format
format:
	clang-format -i -style=file $(shell find $(SOURCE_DIR) -name *.hpp -o -name *.cpp)

.PHONY: build
build:
	mkdir -p ${BUILD_DIR}
	cd ${BUILD_DIR}; cmake ..; make

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
