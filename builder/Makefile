TOP := ..

include $(TOP)/mk/boilerplate.mk

MODULE := builder
BUILD_MODULE_DIR := $(BUILD_DIR)/$(MODULE)

CXX_SRCS := \
	crypto.cpp \
	filesystem.cpp \
	logging.cpp \
	servercommon.cpp \
	stringutils.cpp

include $(TOP)/mk/module-target.mk

BUILDER_OBJS := $(OBJS)
