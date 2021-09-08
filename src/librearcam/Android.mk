LOCAL_PATH		:= $(call my-dir)

include $(CLEAR_VARS)

ANDROID_VERSION_STR := $(PLATFORM_VERSION)
ANDROID_VERSION := $(firstword $(ANDROID_VERSION_STR))

#########################################################################
#																		#
#								Sources									#
#																		#
#########################################################################
LOCAL_C_INCLUDES        += \
	external/libdrm \
	external/libdrm/include/drm  \
	hardware/nexell/s5pxx18/gralloc	\
	linux/platform/$(TARGET_CPU_VARIANT2)/library/include	\
	frameworks/native/include\
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../../include \
	$(LOCAL_PATH)

ifeq ($(ANDROID_VERSION), 9)
LOCAL_C_INCLUDES        += \
	vendor/nexell/library/nx-video-api/src \
	vendor/nexell/library/nx-renderer/include
else
LOCAL_C_INCLUDES        += \
	device/nexell/library/nx-video-api/src \
	device/nexell/library/nx-renderer/include
endif


#########################################################################
#																		#
#								Includes								#
#																		#
#########################################################################
LOCAL_SRC_FILES			:=  \
	NX_CV4l2Camera.cpp \
	NX_CV4l2VipFilter.cpp \
	NX_CFlipFilter.cpp \
	NX_CDeinterlaceFilter.cpp \
	NX_CVideoRenderFilter.cpp \
	nxp_video_alloc_drm.c \

#BackGear Detection
LOCAL_SRC_FILES			+= \
	NX_CGpioControl.cpp \
	NX_CBackgearDetect.cpp \

#Utils
LOCAL_SRC_FILES			+= \
	../common/NX_DbgMsg.cpp \
	../common/NX_ThreadUtils.cpp \

# Manager
LOCAL_SRC_FILES			+= \
	NX_CRearCamManager.cpp \

#########################################################################
#																		#
#								Build optios							#
#																		#
#########################################################################
LOCAL_CFLAGS	+= -DTHUNDER_DEINTERLACE_TEST -DADJUST_THREAD_PRIORITY

ifdef	QUICKBOOT
ifneq ($(ANDROID_VERSION), 9)
LOCAL_CFLAGS += -DDEV_QUICK
endif
endif


#########################################################################
#																		#
#								Target									#
#																		#
#########################################################################
LOCAL_MODULE		:= libnxquickrearcam
LOCAL_MODULE_PATH	:= $(LOCAL_PATH)

LOCAL_MODULE_TAGS	:= optional

include $(BUILD_STATIC_LIBRARY)
