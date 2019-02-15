LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -DCAN_COMPILE_ARM -D__ARM_NEON__
LOCAL_CFLAGS += -DUSE_ION_ALLOCATOR

LOCAL_SRC_FILES := \
	merge.c \
	merge_arm.s \
	algo_basic.c \
	algo_x.c \
	nx_deinterlace.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/./include \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)

LOCAL_C_INCLUDES += \
	hardware/nexell/s5pxx18/gralloc

LOCAL_LDFLAGS +=

LOCAL_MODULE := libnxdeinterlace

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)
