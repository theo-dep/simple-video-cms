TOP := ..

vpath %.cpp $(TOP)/builder
vpath %.c $(TOP)/builder/third-party

ZLIB_DIR := $(TOP)/builder/third-party/zlib
LIBPNG_DIR := $(TOP)/builder/third-party/libpng
FFMPEG_DIR := $(TOP)/builder/third-party/ffmpeg

ZLIB_LIBS := z
LIBPNG_LIBS := png16
FFMPEG_LIBS := avformat avcodec avdevice avfilter swresample swscale avutil

override INC_DIRS += $(FFMPEG_DIR) $(LIBPNG_DIR)
override LDFLAGS += \
	$(addprefix -L,$(ZLIB_DIR) $(LIBPNG_DIR)/.libs $(addprefix $(FFMPEG_DIR)/lib,$(FFMPEG_LIBS))) \
	$(addprefix -l,$(FFMPEG_LIBS) $(LIBPNG_LIBS) $(ZLIB_LIBS))

include $(TOP)/mk/boilerplate.mk

include $(TOP)/builder/Makefile

MODULE := back
BUILD_MODULE_DIR := $(BUILD_DIR)/$(MODULE)
BIN_MODULE_DIR := $(BIN_DIR)/$(MODULE)

CC_SRCS := \
	sqlite3.c

CXX_SRCS := \
	database.cpp \
	main.cpp \
	search.cpp \
	server.cpp \
	video.cpp

ZLIB_TARGETS := $(addprefix $(ZLIB_DIR)/lib,$(addsuffix .a,$(ZLIB_LIBS)))
LIBPNG_TARGETS := $(addprefix $(LIBPNG_DIR)/.libs/lib,$(addsuffix .a,$(LIBPNG_LIBS)))
FFMPEG_TARGETS := $(join $(addprefix $(FFMPEG_DIR)/lib,$(FFMPEG_LIBS)),$(addprefix /lib,$(addsuffix .a,$(FFMPEG_LIBS))))

LIBS_TARGETS := $(ZLIB_TARGETS) $(LIBPNG_TARGETS) $(FFMPEG_TARGETS)

include $(TOP)/mk/target.mk

.PHONY: zlib libpng ffmpeg
zlib: $(ZLIB_TARGETS)
$(ZLIB_TARGETS):
	@$(MAKE) -C $(dir $@)
# .libs directory does not exist before build
libpng: $(LIBPNG_TARGETS)
$(LIBPNG_TARGETS):
	-@mkdir -p $(dir $@)
	@$(MAKE) -C $(dir $@)/..
# build all but make only one target to not execute multiple make
ffmpeg: $(firstword $(FFMPEG_TARGETS))
$(firstword $(FFMPEG_TARGETS)):
	@$(MAKE) -C $(dir $@)/..

# link with builder objects
OBJS := $(OBJS) $(BUILDER_OBJS)
$(BIN_MODULE_DIR)/$(TARGET): zlib libpng ffmpeg $(BUILDER_OBJS)

clean::
	-@$(MAKE) -C $(ZLIB_DIR) clean
	-@$(MAKE) -C $(LIBPNG_DIR) clean
	-@$(MAKE) -C $(FFMPEG_DIR) clean

distclean::
	-@$(MAKE) -C $(ZLIB_DIR) distclean
	-@$(MAKE) -C $(LIBPNG_DIR) distclean
	-@$(MAKE) -C $(FFMPEG_DIR) distclean
