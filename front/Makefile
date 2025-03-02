TOP := ..

vpath %.cpp $(TOP)/common

include $(TOP)/mk/boilerplate.mk

include $(TOP)/common/Makefile

MODULE := front
BUILD_MODULE_DIR := $(BUILD_DIR)/$(MODULE)
BIN_MODULE_DIR := $(BIN_DIR)/$(MODULE)

CXX_SRCS := \
	client.cpp \
	confirmhandler.cpp \
	main.cpp \
	server.cpp \
	session.cpp

include $(TOP)/mk/target.mk

# link with common objects
OBJS := $(OBJS) $(BUILDER_OBJS)
$(BIN_MODULE_DIR)/$(TARGET): $(LIBS_TARGETS) $(BUILDER_OBJS)
