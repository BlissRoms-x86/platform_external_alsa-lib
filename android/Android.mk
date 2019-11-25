#
# Copyright (C) 2019 The Android-x86 Open Source Project
#
# Licensed under the GNU Lesser General Public License Version 2.1.
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.gnu.org/licenses/lgpl.html
#

LOCAL_PATH := $(dir $(call my-dir))
include $(CLEAR_VARS)

LOCAL_MODULE := libasound
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

intermediates := $(call local-generated-sources-dir)

LOCAL_C_INCLUDES := $(LOCAL_PATH)include $(LOCAL_PATH)android
LOCAL_EXPORT_C_INCLUDE_DIRS := $(intermediates) $(LOCAL_PATH)

LOCAL_CFLAGS := -DPIC \
	-Wno-absolute-value -Wno-address-of-packed-member -Wno-missing-braces \
	-Wno-pointer-arith -Wno-sign-compare -Wno-unused-function \
	-Wno-unused-const-variable -Wno-unused-parameter -Wno-unused-variable \
	-finline-limit=300 -finline-functions -fno-inline-functions-called-once \
	-Wno-implicit-function-declaration

# list of files to be excluded
EXCLUDE_SRC_FILES := \
	src/alisp/alisp_snd.c \
	src/compat/hsearch_r.c \
	src/control/control_shm.c \
	src/pcm/pcm_d%.c \
	src/pcm/pcm_ladspa.c \
	src/pcm/pcm_shm.c \
	src/pcm/scopes/level.c \

LOCAL_SRC_FILES := $(filter-out $(EXCLUDE_SRC_FILES),$(call all-c-files-under,src))

GEN := $(intermediates)/alsa/asoundlib.h
$(GEN): $(LOCAL_PATH)configure.ac $(wildcard $(LOCAL_PATH)include/*.h)
	rm -rf $(@D); mkdir -p $(@D); cd $(@D); \
	ln -s ../../../../../../../../$(<D)/include/*.h .
	cat $(@D)/asoundlib-head.h > $@; \
	sed -n "/.*\(#include <[ae].*.h>\).*/s//\1/p" $< >> $@; \
	cat $(@D)/asoundlib-tail.h >> $@
	sed -n "/^AC_INIT.* \([0-9.]*\))/s//\#define SND_LIB_VERSION_STR \"\1\"/p" $< > $(@D)/version.h; \
	ln -sf alsa/version.h $(@D)/..

LOCAL_GENERATED_SOURCES := $(GEN)

LOCAL_SHARED_LIBRARIES := libdl

include $(BUILD_SHARED_LIBRARY)
