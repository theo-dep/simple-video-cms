TOP := ..

include $(TOP)/mk/module-target.mk

.PHONY: all
all: $(BIN_MODULE_DIR)/$(TARGET)

$(BIN_MODULE_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

.PHONY: start
start:
	@cd $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
	$(BIN_MODULE_DIR)/$(TARGET)
