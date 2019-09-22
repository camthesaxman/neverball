### Neverball makefile for Android ###

# Directory containing this Android.mk file
JNI_DIR := $(call my-dir)

# Path to extracted library sources
LIBSDL2_PATH   := $(JNI_DIR)/SDL2-2.0.10
LIBPNG_PATH    := $(JNI_DIR)/libpng-android
LIBJPEG_PATH   := $(JNI_DIR)/libjpeg-turbo
LIBVORBIS_PATH := $(JNI_DIR)/libvorbis-1.3.6
LIBOGG_PATH    := $(JNI_DIR)/libogg-1.3.4

# Build all necessary libraries
include $(call all-subdir-makefiles)
include $(LIBPNG_PATH)/jni/Android.mk


# Neverball

# Path to neverball repository
NEVERBALL_ROOT_PATH := $(JNI_DIR)/../../..

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_SRC_FILES := \
	$(NEVERBALL_ROOT_PATH)/share/lang.c        \
	$(NEVERBALL_ROOT_PATH)/share/st_common.c   \
	$(NEVERBALL_ROOT_PATH)/share/vec3.c        \
	$(NEVERBALL_ROOT_PATH)/share/base_image.c  \
	$(NEVERBALL_ROOT_PATH)/share/image.c       \
	$(NEVERBALL_ROOT_PATH)/share/solid_base.c  \
	$(NEVERBALL_ROOT_PATH)/share/solid_vary.c  \
	$(NEVERBALL_ROOT_PATH)/share/solid_draw.c  \
	$(NEVERBALL_ROOT_PATH)/share/solid_all.c   \
	$(NEVERBALL_ROOT_PATH)/share/mtrl.c        \
	$(NEVERBALL_ROOT_PATH)/share/part.c        \
	$(NEVERBALL_ROOT_PATH)/share/geom.c        \
	$(NEVERBALL_ROOT_PATH)/share/ball.c        \
	$(NEVERBALL_ROOT_PATH)/share/gui.c         \
	$(NEVERBALL_ROOT_PATH)/share/font.c        \
	$(NEVERBALL_ROOT_PATH)/share/theme.c       \
	$(NEVERBALL_ROOT_PATH)/share/base_config.c \
	$(NEVERBALL_ROOT_PATH)/share/config.c      \
	$(NEVERBALL_ROOT_PATH)/share/video.c       \
	$(NEVERBALL_ROOT_PATH)/share/glext.c       \
	$(NEVERBALL_ROOT_PATH)/share/binary.c      \
	$(NEVERBALL_ROOT_PATH)/share/state.c       \
	$(NEVERBALL_ROOT_PATH)/share/audio.c       \
	$(NEVERBALL_ROOT_PATH)/share/text.c        \
	$(NEVERBALL_ROOT_PATH)/share/common.c      \
	$(NEVERBALL_ROOT_PATH)/share/list.c        \
	$(NEVERBALL_ROOT_PATH)/share/queue.c       \
	$(NEVERBALL_ROOT_PATH)/share/cmd.c         \
	$(NEVERBALL_ROOT_PATH)/share/array.c       \
	$(NEVERBALL_ROOT_PATH)/share/dir.c         \
	$(NEVERBALL_ROOT_PATH)/share/fbo.c         \
	$(NEVERBALL_ROOT_PATH)/share/glsl.c        \
	$(NEVERBALL_ROOT_PATH)/share/fs_common.c   \
	$(NEVERBALL_ROOT_PATH)/share/fs_png.c      \
	$(NEVERBALL_ROOT_PATH)/share/fs_jpg.c      \
	$(NEVERBALL_ROOT_PATH)/share/fs_ov.c       \
	$(NEVERBALL_ROOT_PATH)/share/log.c         \
	$(NEVERBALL_ROOT_PATH)/share/joy.c         \
	$(NEVERBALL_ROOT_PATH)/ball/hud.c          \
	$(NEVERBALL_ROOT_PATH)/ball/game_common.c  \
	$(NEVERBALL_ROOT_PATH)/ball/game_client.c  \
	$(NEVERBALL_ROOT_PATH)/ball/game_server.c  \
	$(NEVERBALL_ROOT_PATH)/ball/game_proxy.c   \
	$(NEVERBALL_ROOT_PATH)/ball/game_draw.c    \
	$(NEVERBALL_ROOT_PATH)/ball/score.c        \
	$(NEVERBALL_ROOT_PATH)/ball/level.c        \
	$(NEVERBALL_ROOT_PATH)/ball/progress.c     \
	$(NEVERBALL_ROOT_PATH)/ball/set.c          \
	$(NEVERBALL_ROOT_PATH)/ball/demo.c         \
	$(NEVERBALL_ROOT_PATH)/ball/demo_dir.c     \
	$(NEVERBALL_ROOT_PATH)/ball/util.c         \
	$(NEVERBALL_ROOT_PATH)/ball/st_conf.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_demo.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_save.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_goal.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_fail.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_done.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_level.c     \
	$(NEVERBALL_ROOT_PATH)/ball/st_over.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_play.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_set.c       \
	$(NEVERBALL_ROOT_PATH)/ball/st_start.c     \
	$(NEVERBALL_ROOT_PATH)/ball/st_title.c     \
	$(NEVERBALL_ROOT_PATH)/ball/st_help.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_name.c      \
	$(NEVERBALL_ROOT_PATH)/ball/st_shared.c    \
	$(NEVERBALL_ROOT_PATH)/ball/st_pause.c     \
	$(NEVERBALL_ROOT_PATH)/ball/st_ball.c      \
	$(NEVERBALL_ROOT_PATH)/ball/main.c         \
	$(NEVERBALL_ROOT_PATH)/share/solid_sim_sol.c \
	$(NEVERBALL_ROOT_PATH)/share/fs_stdio.c    \
	$(NEVERBALL_ROOT_PATH)/share/tilt_android.c   \
	$(NEVERBALL_ROOT_PATH)/share/hmd_null.c

# Build libogg and libvorbis into the main module, since they don't support the
# Android build system
LOCAL_SRC_FILES += \
	$(LIBOGG_PATH)/src/framing.c \
	$(LIBOGG_PATH)/src/bitwise.c
LOCAL_SRC_FILES += \
	$(LIBVORBIS_PATH)/lib/bitrate.c    \
	$(LIBVORBIS_PATH)/lib/block.c      \
	$(LIBVORBIS_PATH)/lib/codebook.c   \
	$(LIBVORBIS_PATH)/lib/envelope.c   \
	$(LIBVORBIS_PATH)/lib/floor0.c     \
	$(LIBVORBIS_PATH)/lib/floor1.c     \
	$(LIBVORBIS_PATH)/lib/info.c       \
	$(LIBVORBIS_PATH)/lib/lpc.c        \
	$(LIBVORBIS_PATH)/lib/lsp.c        \
	$(LIBVORBIS_PATH)/lib/mapping0.c   \
	$(LIBVORBIS_PATH)/lib/mdct.c       \
	$(LIBVORBIS_PATH)/lib/psy.c        \
	$(LIBVORBIS_PATH)/lib/registry.c   \
	$(LIBVORBIS_PATH)/lib/res0.c       \
	$(LIBVORBIS_PATH)/lib/sharedbook.c \
	$(LIBVORBIS_PATH)/lib/smallft.c    \
	$(LIBVORBIS_PATH)/lib/synthesis.c  \
	$(LIBVORBIS_PATH)/lib/vorbisfile.c \
	$(LIBVORBIS_PATH)/lib/window.c
LOCAL_CFLAGS := -DENABLE_OPENGLES=1 -DENABLE_FS=stdio
LOCAL_C_INCLUDES := \
	$(NEVERBALL_ROOT_PATH)/share \
	$(LIBSDL2_PATH)/include      \
	$(LIBPNG_PATH)/jni           \
	$(LIBJPEG_PATH)              \
	$(LIBJPEG_PATH)/android      \
	$(LIBVORBIS_PATH)/include    \
	$(LIBOGG_PATH)/include
LOCAL_SHARED_LIBRARIES := hidapi SDL2 SDL2_ttf libjpeg
LOCAL_STATIC_LIBRARIES := libpng
LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -llog -landroid

# Generate headers for neverball
BUILD := $(shell head -n1 $(NEVERBALL_ROOT_PATH)/.build 2> /dev/null || echo release)
VERSION := 1.6.0
VERSION := $(shell sh $(NEVERBALL_ROOT_PATH)/scripts/version.sh "$(BUILD)" "$(VERSION)" "$(NEVERBALL_ROOT_PATH)/share/version.in.h" "$(NEVERBALL_ROOT_PATH)/share/version.h" "$(NEVERBALL_ROOT_PATH)/.version")

# Generate headers for libogg
DUMMY := $(shell sed \
	-e 's/@INCLUDE_INTTYPES_H@/1/g'  \
	-e 's/@INCLUDE_STDINT_H@/1/g'    \
	-e 's/@INCLUDE_SYS_TYPES_H@/1/g' \
	-e 's/@SIZE16@/int16_t/g'        \
	-e 's/@USIZE16@/uint16_t/g'      \
	-e 's/@SIZE32@/int32_t/g'        \
	-e 's/@USIZE32@/uint32_t/g'      \
	-e 's/@SIZE64@/int64_t/g'        \
	-e 's/@USIZE64@/uint64_t/g'      \
	$(LIBOGG_PATH)/include/ogg/config_types.h.in > $(LIBOGG_PATH)/include/ogg/config_types.h)

# Build data files
#DUMMY := $(shell make -C $(NEVERBALL_ROOT_PATH) sols)

include $(BUILD_SHARED_LIBRARY)