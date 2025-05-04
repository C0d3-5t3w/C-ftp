.PHONY: all clean build rebuild install

BUILD_DIR = build

all: build

build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

rebuild: clean build

install: build
	cd $(BUILD_DIR) && make install

clean:
	rm -rf $(BUILD_DIR)
