CC ?= gcc
BASE_CFLAGS ?= -std=c23 -Wall -Wextra -Wpedantic -Wconversion \
	-Wdouble-promotion -Wno-unused-parameter -Wno-unused-function \
	-Wno-format-extra-args -Wno-format
DEBUG_CFLAGS := $(BASE_CFLAGS) -fsanitize=address,undefined -g3 -DASSERT_TRAP
COVERAGE_CFLAGS := $(BASE_CFLAGS) --coverage -fprofile-abs-path
RELEASE_CFLAGS := $(BASE_CFLAGS) -O2
programs := $(patsubst programs/%.c,$(BUILD_DIR)/bin/%,$(wildcard programs/*.c))

.PHONY: all debug release coverage afl test fuzz dist build clean distclean

all: release

debug:
	$(MAKE) BUILD_DIR=build/debug CFLAGS='$(DEBUG_CFLAGS)' build

release:
	$(MAKE) BUILD_DIR=build/release CFLAGS='$(RELEASE_CFLAGS)' build

coverage:
	$(MAKE) BUILD_DIR=build/coverage CFLAGS='$(COVERAGE_CFLAGS)' build

afl:
	$(MAKE) CC=afl-cc BUILD_DIR=build/afl CFLAGS='$(RELEASE_CFLAGS)' build

test: debug
	./test/test.sh

fuzz: afl
	./test/fuzz.sh

dist: release
	./dist.sh

build: $(BUILD_DIR)/libgraf.a $(programs)

clean:
	rm -rf build

distclean:
	rm -rf dist graf-*.tar.gz

$(BUILD_DIR)/libgraf.a: graf.c graf.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c graf.c -o $(BUILD_DIR)/graf.o
	ar rcs $@ $(BUILD_DIR)/graf.o

$(BUILD_DIR)/bin/%: programs/%.c $(BUILD_DIR)/libgraf.a
	mkdir -p $(BUILD_DIR)/bin
	$(CC) $(CFLAGS) -I. $^ -o $@