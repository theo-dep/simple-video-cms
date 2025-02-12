# inspired from https://makefiletutorial.com/#makefile-cookbook
# and https://downloads.haskell.org/ghc/5.02.2/docs/building/sec-makefile-arch.html

TOP := ..

override CFLAGS += -Wall -Wextra -Werror
override INC_DIRS += $(TOP)/builder $(TOP)/builder/third-party
override CXXFLAGS += -std=c++23 -Wall -Wextra -Werror -DCPPHTTPLIB_USE_POLL
override LDFLAGS += -static -lstdc++exp

INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS := $(INC_FLAGS) -MMD -MP

ANALYZER := clang-tidy

TARGET := server

BASE_BUILD_DIR := $(TOP)/build
BUILD_DIR := $(BASE_BUILD_DIR)

BASE_BIN_DIR := $(TOP)/bin
BIN_DIR := $(BASE_BIN_DIR)
