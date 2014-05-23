LOCAL_PATH := $(call my-dir)
ocpa_INCLUDES := 
subproject_PATHS := \
					router \
					utils \
					lzo \
					snappy \
					openssl \
					openvpn \
					blinkt \
					ocpa

include $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, \
		$(subproject_PATHS)))