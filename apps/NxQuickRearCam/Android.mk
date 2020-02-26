LOCAL_PATH		:= $(call my-dir)

NX_INC_TOP		:= $(LOCAL_PATH)/../../include
NX_LIB_TOP		:= $(LOCAL_PATH)/../../lib/android


include $(CLEAR_VARS)
LOCAL_MODULE    := libdeinterlacer_static
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_C_INCLUDE := $(NX_INC_TOP)
LOCAL_SRC_FILES := ../../lib/android/libdeinterlacer_static.a
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)

#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
LOCAL_C_INCLUDES +=	\
	external/libdrm \
	hardware/nexell/s5pxx18/gralloc	\
	$(NX_INC_TOP) \
	$(NX_INC_TOP)/drm \
	$(NX_INC_TOP)/../src/common \
	$(LOCAL_PATH)

#########################################################################
#																		#
#								Sources									#
#																		#
#########################################################################
LOCAL_SRC_FILES		+=		\
	NX_DbgMsg.cpp \
	NX_CCommand.cpp \
	NX_CRGBDraw.cpp \
	NX_QuickRearCam.cpp \
	../../src/common/NX_ThreadUtils.cpp \

#########################################################################
#																		#
#								Library									#
#																		#
#########################################################################

LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_STATIC_LIBRARIES += liblog libdrm

LOCAL_STATIC_LIBRARIES += libnxquickrearcam libnxdeinterlace libnx_renderer

ifdef	QUICKBOOT
LOCAL_STATIC_LIBRARIES += libnx_v4l2_q
endif

ifndef QUICKBOOT
LOCAL_STATIC_LIBRARIES += libnx_v4l2
endif

LOCAL_STATIC_LIBRARIES += libdeinterlacer_static


#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
LOCAL_MODULE		:= NxQuickRearCam
LOCAL_MODULE_TAGS	:= optional
LOCAL_CFLAGS 		:= -DANDROID -static -pthread -DADJUST_THREAD_PRIORITY
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_SBIN_UNSTRIPPED)
LOCAL_POST_INSTALL_CMD := $(hide) mkdir -p $(TARGET_ROOT_OUT)/sbin

include $(BUILD_EXECUTABLE)