CC ?= gcc
BASE_CFLAGS ?= -std=c23 -Wall -Wextra -Wpedantic -Wconversion \
	-Wdouble-promotion -Wno-unused-parameter -Wno-unused-function
DEBUG_CFLAGS := $(BASE_CFLAGS) -g3 -fsanitize=address,undefined
RELEASE_CFLAGS := $(BASE_CFLAGS) -O2

.PHONY: all debug release afl test fuzz build clean

all: release

debug:
	$(MAKE) BUILD_DIR=build/debug CFLAGS='$(DEBUG_CFLAGS)' build

release:
	$(MAKE) BUILD_DIR=build/release CFLAGS='$(RELEASE_CFLAGS)' build

afl:
	$(MAKE) CC=afl-cc BUILD_DIR=build/afl CFLAGS='$(RELEASE_CFLAGS)' build

test: release
	./test/test.sh

fuzz: afl
	./test/fuzz.sh

build: $(BUILD_DIR)/libgraf.a $(BUILD_DIR)/graf-info

clean:
	rm -rf build

$(BUILD_DIR)/libgraf.a: graf.c graf.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c graf.c -o $(BUILD_DIR)/graf.o
	ar rcs $@ $(BUILD_DIR)/graf.o

$(BUILD_DIR)/graf-info: graf-info.c $(BUILD_DIR)/libgraf.a
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) graf-info.c $(BUILD_DIR)/libgraf.a -o $@
