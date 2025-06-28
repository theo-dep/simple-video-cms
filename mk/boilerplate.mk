# inspired from https://makefiletutorial.com/#makefile-cookbook
# and https://downloads.haskell.org/ghc/5.02.2/docs/building/sec-makefile-arch.html

ifndef INCLUDED
INCLUDED = 1

TOP := ..

override RELEASE_FLAGS += -O3 -DNDEBUG
override DEBUG_FLAGS += -g -D_DEBUG -DDEBUG_LOG

override INC_DIRS += $(TOP)/common $(TOP)/common/third-party
override CFLAGS += -Wall -Wextra -Werror
override CXXFLAGS += -std=c++23 -Wall -Wextra -Werror -DCPPHTTPLIB_USE_POLL
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

ANALYZER := clang-tidy

TARGET := server

BASE_BUILD_DIR := $(TOP)/build
BUILD_DIR := $(BASE_BUILD_DIR)/$(BUILD)

BASE_BIN_DIR := $(TOP)/bin
BIN_DIR := $(BASE_BIN_DIR)/$(BUILD)

endif
