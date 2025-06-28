TOP := ..

ZLIB_DIR := $(TOP)/common/third-party/zlib

ZLIB_LIBS := z

override INC_DIRS += $(ZLIB_DIR)
override LDFLAGS += -L$(ZLIB_DIR) -l$(ZLIB_LIBS)

ZLIB_TARGETS := $(addprefix $(ZLIB_DIR)/lib,$(addsuffix .a,$(ZLIB_LIBS)))

.PHONY: zlib
zlib: $(ZLIB_TARGETS)
$(ZLIB_TARGETS):
	@$(MAKE) -C $(dir $@)

clean::
	-@$(MAKE) -C $(ZLIB_DIR) clean

distclean::
	-@$(MAKE) -C $(ZLIB_DIR) distclean
