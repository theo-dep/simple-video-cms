TOP := ..

vpath %.cpp $(TOP)/common
vpath %.c $(TOP)/common/third-party

LIBPNG_DIR := $(TOP)/common/third-party/libpng
FFMPEG_DIR := $(TOP)/common/third-party/ffmpeg

LIBPNG_LIBS := png16
FFMPEG_LIBS := avformat avcodec avdevice avfilter swresample swscale avutil

override INC_DIRS += $(FFMPEG_DIR) $(LIBPNG_DIR)
override LDFLAGS += \
	$(addprefix -L,$(LIBPNG_DIR)/.libs $(addprefix $(FFMPEG_DIR)/lib,$(FFMPEG_LIBS))) \
	$(addprefix -l,$(FFMPEG_LIBS) $(LIBPNG_LIBS))

# sqlite3.c: error: octal literals without a '0o' prefix are deprecated
override CFLAGS += -Wno-deprecated-octal-literals

include $(TOP)/mk/zlib.mk
include $(TOP)/mk/boilerplate.mk

MODULE_MAKEFILE_GUARD := defined # do not include clean and analyze from common/Makefile
include $(TOP)/common/Makefile
MODULE_MAKEFILE_GUARD :=

MODULE := back
BUILD_MODULE_DIR := $(BUILD_DIR)/$(MODULE)
BIN_MODULE_DIR := $(BIN_DIR)/$(MODULE)

CC_SRCS := \
	sqlite3.c

CXX_SRCS := \
	database.cpp \
	logcontroller.cpp \
	main.cpp \
	search.cpp \
	server.cpp \
	video.cpp

LIBPNG_TARGETS := $(addprefix $(LIBPNG_DIR)/.libs/lib,$(addsuffix .a,$(LIBPNG_LIBS)))
FFMPEG_TARGETS := $(join $(addprefix $(FFMPEG_DIR)/lib,$(FFMPEG_LIBS)),$(addprefix /lib,$(addsuffix .a,$(FFMPEG_LIBS))))

include $(TOP)/mk/target.mk

.PHONY: libpng ffmpeg
# .libs directory does not exist before build
libpng: $(LIBPNG_TARGETS)
$(LIBPNG_TARGETS):
	-@mkdir -p $(dir $@)
	@$(MAKE) -C $(dir $@)/..
# build all but make only one target to not execute multiple make
ffmpeg: $(firstword $(FFMPEG_TARGETS))
$(firstword $(FFMPEG_TARGETS)):
	@$(MAKE) -C $(dir $@)/..

# dependency to pnglibconf.h
video.cpp: libpng

# link with common objects
OBJS := $(OBJS) $(BUILDER_OBJS)
$(BIN_MODULE_DIR)/$(TARGET): zlib libpng ffmpeg $(BUILDER_OBJS)

clean::
	-@$(MAKE) -C $(LIBPNG_DIR) clean
	-@$(MAKE) -C $(FFMPEG_DIR) clean

distclean::
	-@$(MAKE) -C $(LIBPNG_DIR) distclean
	-@$(MAKE) -C $(FFMPEG_DIR) distclean
