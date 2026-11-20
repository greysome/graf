CC ?= gcc
BASE_CFLAGS ?= -std=c23 -lm -Wall -Wextra -Wpedantic -Wconversion \
	-Wdouble-promotion -Wno-unused-parameter -Wno-unused-function \
	-Wno-format-extra-args -Wno-format
DEBUG_CFLAGS := $(BASE_CFLAGS) -fsanitize=address,undefined -g3 -DASSERT_TRAP
COVERAGE_CFLAGS := $(BASE_CFLAGS) --coverage -fprofile-abs-path
RELEASE_CFLAGS := $(BASE_CFLAGS) -O2

BASE_ODIN_FLAGS := -collection:graf=./odin -file
DEBUG_ODIN_FLAGS := $(BASE_ODIN_FLAGS) -extra-assembler-flags:"-fsanitize=address,undefined" -debug
DEBUG_ODIN_LINK_FLAGS = -fsanitize=address,undefined

c_programs := $(patsubst programs/%.c,$(BUILD_DIR)/bin/%,$(wildcard programs/*.c))
odin_programs := $(patsubst programs/%.odin,$(BUILD_DIR)/bin/%,$(wildcard programs/*.odin))

.PHONY: all debug release coverage afl test fuzz dist build clean distclean

all: release

debug:
	$(MAKE) BUILD_DIR=build/debug CFLAGS='$(DEBUG_CFLAGS)' ODIN_FLAGS='$(DEBUG_ODIN_FLAGS)' ODIN_LINK_FLAGS='$(DEBUG_ODIN_LINK_FLAGS)' build

release:
	$(MAKE) BUILD_DIR=build/release CFLAGS='$(RELEASE_CFLAGS)' ODIN_FLAGS='$(BASE_ODIN_FLAGS)' build

coverage:
	$(MAKE) BUILD_DIR=build/coverage CFLAGS='$(COVERAGE_CFLAGS)' ODIN_FLAGS='$(BASE_ODIN_FLAGS)' build

afl:
	$(MAKE) CC=afl-cc BUILD_DIR=build/afl CFLAGS='$(RELEASE_CFLAGS)' ODIN_FLAGS='$(BASE_ODIN_FLAGS)' build

test: debug
	./test/test.sh

fuzz: afl
	./test/fuzz.sh

dist: release
	./dist.sh

build: $(BUILD_DIR)/libgraf.a $(c_programs) $(odin_programs)

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

$(BUILD_DIR)/bin/%: programs/%.odin $(BUILD_DIR)/libgraf.a
	mkdir -p $(BUILD_DIR)/bin
	odin build $< $(ODIN_FLAGS) -extra-linker-flags:"$(ODIN_LINK_FLAGS) -L$(BUILD_DIR)" -out:$@