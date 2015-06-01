LOCAL_PATH:= $(call my-dir)

# yamahaspk control native

include $(CLEAR_VARS)

LOCAL_MODULE        := libjni_yamahaspk

LOCAL_NDK_STL_VARIANT := stlport_static

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/src 

#LOCAL_SHARED_LIBRARIES := libyamaha

LOCAL_LDFLAGS        := -llog
LOCAL_SDK_VERSION   := 9
LOCAL_ARM_MODE := arm

LOCAL_CFLAGS    += -ffast-math -O3 -funroll-loops
LOCAL_CPPFLAGS += $(JNI_CFLAGS)


LOCAL_CPP_EXTENSION := .cpp
LOCAL_SRC_FILES     := \
    src/yamahaspk_jni.cpp \
    src/yamahaspk.c



include $(BUILD_SHARED_LIBRARY)
