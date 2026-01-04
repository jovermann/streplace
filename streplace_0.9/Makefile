# Copyright (c) 2022 Johannes Overmann
# Released under the MIT license. See LICENSE for license.

TARGET = streplace

CPPFLAGS ?=

CXXFLAGS ?= -Wall -Wextra -O3

CXXSTD ?= -std=c++23

SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:%.cpp=build/%.o)
DEPENDS := $(SOURCES:%.cpp=build/%.d)

default: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

build/%.o: %.cpp build/%.d
	$(CXX) $(CXXSTD) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
        
build/%.d: %.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CXXSTD) $(CPPFLAGS) -MM -MQ $@ $< -o $@

clean:
	rm -rf build $(TARGET)
	find . -name '*~' -delete

.PHONY: clean default

ifeq ($(findstring $(MAKECMDGOALS),clean),)
-include $(DEPENDS)
endif
