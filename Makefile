BUILD_DIR := build
BUILD_TYPE := Release
JOBS := $(shell sysctl -n hw.ncpu 2>/dev/null || nproc)

.PHONY: configure build run clean

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR) -j $(JOBS)

run: build
	librespot --name "Speaker Dev (Mac)" --backend pipe --bitrate 160 --onevent "$(CURDIR)/now_playing.sh" \
	| ./$(BUILD_DIR)/apps/speaker/speaker

clean:
	rm -rf $(BUILD_DIR)
