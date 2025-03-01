TOP := ..

OBJS := $(CXX_SRCS:%=$(BUILD_MODULE_DIR)/%.o) $(CC_SRCS:%=$(BUILD_MODULE_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

$(BUILD_MODULE_DIR)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_MODULE_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEPS)

ifeq ($(MODULE_MAKEFILE_GUARD),)
MODULE_MAKEFILE_GUARD := defined

.PHONY: analyze
analyze: $(CXX_SRCS)
	$(ANALYZER) $^ -- $(CPPFLAGS) $(CXXFLAGS)

.PHONY: clean
clean::
	$(RM) -r $(BASE_BUILD_DIR)
	$(RM) -r $(BASE_BIN_DIR)

endif
