# inspired from https://makefiletutorial.com/#makefile-cookbook
# and https://downloads.haskell.org/ghc/5.02.2/docs/building/sec-makefile-arch.html

ifndef INCLUDED
INCLUDED = 1

TOP := ..

CLANG_VERSION := $(shell which clang && clang --version | head -n1)
ifneq (,$(findstring 21,$(CLANG_VERSION)))
    override CC = clang
    override CXX = clang++
else
    override CC = clang-21
    override CXX = clang++-21
endif

override RELEASE_FLAGS += -O3 -DNDEBUG
override DEBUG_FLAGS += -g -D_DEBUG -DDEBUG_LOG

override INC_DIRS += $(TOP)/common $(TOP)/common/third-party
override CFLAGS += -std=c2y -Wall -Wextra -Werror -pedantic
override CXXFLAGS += -std=c++2c -Wall -Wextra -Werror -pedantic -DCPPHTTPLIB_USE_POLL -DCPPHTTPLIB_ZLIB_SUPPORT
override LDFLAGS += -static -lstdc++exp

DEBUG ?= 1
ifeq ($(DEBUG), 1)
BUILD := debug
override CFLAGS += $(DEBUG_FLAGS)
override CXXFLAGS += $(DEBUG_FLAGS)
else
BUILD := release
override CFLAGS += $(RELEASE_FLAGS)
override CXXFLAGS += $(RELEASE_FLAGS)
endif

INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS := $(INC_FLAGS) -MMD -MP

CLANG_TIDY_VERSION := $(shell which clang-tidy && clang-tidy --version | head -n1)
ifneq (,$(findstring 21,$(CLANG_VERSION)))
    override ANALYZER = clang-tidy
else
    override ANALYZER = clang-tidy-21
endif

TARGET := server

BASE_BUILD_DIR := $(TOP)/build
BUILD_DIR := $(BASE_BUILD_DIR)/$(BUILD)

BASE_BIN_DIR := $(TOP)/bin
BIN_DIR := $(BASE_BIN_DIR)/$(BUILD)

endif
