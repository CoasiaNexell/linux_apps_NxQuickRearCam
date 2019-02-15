LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=	\
	nx-v4l2.c

LOCAL_SRC_FILES += \
	../../src/common/NX_DbgMsg.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../src/common \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)

LOCAL_LDFLAGS	+= \
	-llog	\

LOCAL_MODULE		:= libnxv4l2
LOCAL_MODULE_PATH	:= $(LOCAL_PATH)

LOCAL_MODULE_TAGS	:= optional


include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)
