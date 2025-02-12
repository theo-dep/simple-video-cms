TOP := ..

vpath %.cpp $(TOP)/builder

include $(TOP)/mk/boilerplate.mk

include $(TOP)/builder/Makefile

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

# link with builder objects
OBJS := $(OBJS) $(BUILDER_OBJS)
$(BIN_MODULE_DIR)/$(TARGET): $(LIBS_TARGETS) $(BUILDER_OBJS)
