TOP := ..

vpath %.cpp $(TOP)/common

include $(TOP)/mk/zlib.mk
include $(TOP)/mk/boilerplate.mk

MODULE_MAKEFILE_GUARD := defined # do not include clean and analyze from common/Makefile
include $(TOP)/common/Makefile
MODULE_MAKEFILE_GUARD :=

MODULE := front
BUILD_MODULE_DIR := $(BUILD_DIR)/$(MODULE)
BIN_MODULE_DIR := $(BIN_DIR)/$(MODULE)

CXX_SRCS := \
	client.cpp \
	confirmhandler.cpp \
	cookie.cpp \
	main.cpp \
	server.cpp \
	session.cpp \
	videosession.cpp

include $(TOP)/mk/target.mk

# link with common objects
OBJS := $(OBJS) $(BUILDER_OBJS)
$(BIN_MODULE_DIR)/$(TARGET): zlib $(BUILDER_OBJS)
