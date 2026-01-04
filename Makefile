# Copyright (c) 2021-2022 Johannes Overmann
# Released under the MIT license. See LICENSE for license.

TARGET = streplace

CPPFLAGS ?= -pedantic

#CXXFLAGS ?= -Wall -Wextra
CXXFLAGS ?= -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-shorten-64-to-32 -Wno-missing-prototypes -Wno-sign-conversion -Wno-implicit-int-conversion -Wno-poison-system-directories -fcomment-block-commands=n -Wno-string-conversion -Wno-covered-switch-default

CXXSTD ?= -std=c++23

BUILDDIR=build
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:%.cpp=$(BUILDDIR)/%.o)
DEPENDS := $(SOURCES:%.cpp=$(BUILDDIR)/%.d)

default: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@

build/%.o: %.cpp build/%.d
	$(CXX) $(CXXSTD) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
        
build/%.d: %.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXSTD) $(CPPFLAGS) -MM -MQ $@ $< -o $@

clean:
	rm -rf build $(TARGET) unit_test
	find . -name '*~' -delete

uint_test: clean
unit_test: CPPFLAGS += -D ENABLE_UNIT_TEST
unit_test: CXXFLAGS += -Wno-weak-vtables -Wno-missing-variable-declarations -Wno-exit-time-destructors -Wno-global-constructors
unit_test: $(OBJECTS)
	$(CXX) $^ -o $@
	./unit_test

test: unit_test

format:
	clang-format -i --style=file src/*.hpp src/*.cpp

tidy: CXXFLAGS += -MJ $@.cdb
tidy: $(TARGET)
	echo "[" > $(BUILDDIR)/compile_commands.json
	cat $(BUILDDIR)/src/*.cdb >> $(BUILDDIR)/compile_commands.json
	echo "]" >> $(BUILDDIR)/compile_commands.json
	clang-tidy -p $(BUILDDIR) --config-file .clang-tidy src/*.cpp src/*.hpp

.PHONY: clean default unit_test test format

ifeq ($(findstring $(MAKECMDGOALS),clean),)
-include $(DEPENDS)
endif
