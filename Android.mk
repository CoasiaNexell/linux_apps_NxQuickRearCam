LOCAL_PATH := $(call my-dir)

NX_QUICK_REAR_CAM_TOP := $(LOCAL_PATH)

include $(NX_QUICK_REAR_CAM_TOP)/private/libnxdeinterlace/Android.mk
include $(NX_QUICK_REAR_CAM_TOP)/src/librearcam/Android.mk

include $(NX_QUICK_REAR_CAM_TOP)/apps/NxQuickRearCam/Android.mk