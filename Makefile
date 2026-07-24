# Copyright (c) 2021-2022 Johannes Overmann
# Released under the MIT license. See LICENSE for license.

TARGET = streplace

CPPFLAGS ?= -pedantic

WARNING_FLAGS ?= -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-shorten-64-to-32 -Wno-missing-prototypes -Wno-sign-conversion -Wno-implicit-int-conversion -Wno-poison-system-directories -fcomment-block-commands=n -Wno-string-conversion -Wno-covered-switch-default -Wno-extra-semi-stmt

CXXSTD ?= -std=c++23
UNAME_S := $(shell uname -s)
ifeq ($(origin CXX),default)
ifeq ($(UNAME_S),Linux)
CXX = clang++-18
else
CXX = g++
endif
endif
BUILD ?= release
CXXFLAGS_COMMON ?= -Wall
CXXFLAGS_DEBUG ?= -O0 -g
CXXFLAGS_RELEASE ?= -O3 -DNDEBUG

ifeq ($(BUILD),debug)
CXXFLAGS ?= $(CXXFLAGS_COMMON) $(CXXFLAGS_DEBUG)
else ifeq ($(BUILD),release)
CXXFLAGS ?= $(CXXFLAGS_COMMON) $(CXXFLAGS_RELEASE)
else
$(error Unknown BUILD='$(BUILD)', expected debug or release)
endif

BUILDDIR=build-$(BUILD)
UNIT_TEST_BUILDDIR=build-unit-test-$(BUILD)
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:%.cpp=$(BUILDDIR)/%.o)
DEPENDS := $(SOURCES:%.cpp=$(BUILDDIR)/%.d)
UNIT_TEST_OBJECTS = $(SOURCES:%.cpp=$(UNIT_TEST_BUILDDIR)/%.o)
UNIT_TEST_DEPENDS := $(SOURCES:%.cpp=$(UNIT_TEST_BUILDDIR)/%.d)

default: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@

$(BUILDDIR)/%.o: %.cpp $(BUILDDIR)/%.d
	$(CXX) $(CXXSTD) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
        
$(BUILDDIR)/%.d: %.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXSTD) $(CPPFLAGS) -MM -MQ $@ $< -o $@

clean:
	rm -rf build build-* build-unit-test $(TARGET) unit_test
	find . -name '*~' -delete

$(UNIT_TEST_BUILDDIR)/%.o: %.cpp $(UNIT_TEST_BUILDDIR)/%.d
	$(CXX) $(CXXSTD) $(CPPFLAGS) -D ENABLE_UNIT_TEST $(CXXFLAGS) -c $< -o $@

$(UNIT_TEST_BUILDDIR)/%.d: %.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXSTD) $(CPPFLAGS) -D ENABLE_UNIT_TEST -MM -MQ $@ $< -o $@

unit_test: $(UNIT_TEST_OBJECTS)
	$(CXX) $^ -o $@
	./unit_test

test: unit_test $(TARGET)
	pytest

format:
	clang-format -i --style=file src/*.hpp src/*.cpp

tidy: CXXFLAGS += -MJ $@.cdb
tidy: $(TARGET)
	echo "[" > $(BUILDDIR)/compile_commands.json
	cat $(BUILDDIR)/src/*.cdb >> $(BUILDDIR)/compile_commands.json
	echo "]" >> $(BUILDDIR)/compile_commands.json
	clang-tidy -p $(BUILDDIR) --config-file .clang-tidy src/*.cpp src/*.hpp

warnings:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS_RELEASE) $(WARNING_FLAGS)" $(TARGET)

.PHONY: clean default unit_test test format warnings

ifeq ($(findstring $(MAKECMDGOALS),clean),)
ifneq ($(MAKECMDGOALS),unit_test)
-include $(DEPENDS)
endif
ifneq ($(filter unit_test test,$(MAKECMDGOALS)),)
-include $(UNIT_TEST_DEPENDS)
endif
endif
