
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := mkbootimg.c
LOCAL_STATIC_LIBRARIES := libmincrypt

LOCAL_MODULE := mkbootimg

include $(BUILD_HOST_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_SRC_FILES := unbootimg.c
LOCAL_STATIC_LIBRARIES := libmincrypt

LOCAL_MODULE := unbootimg

include $(BUILD_HOST_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_PREBUILT_EXECUTABLES := unpack.sh repack.sh

include $(BUILD_HOST_PREBUILT)


$(call dist-for-goals,droid,$(LOCAL_BUILT_MODULE))
