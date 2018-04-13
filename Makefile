#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

export PROJECT_PATH := $(PWD)
export IDF_PATH ?= $(PWD)/esp-idf

ifeq ($(SERVER), 1)
	PROJECT_NAME := iotivity_server
else
	PROJECT_NAME := iotivity_client
endif

include $(IDF_PATH)/make/project.mk

# sdkconfig is included project.mk recursively
# after setting sdkconfig done, start user layer macro define  
ifdef CONFIG_IOTIVITY_CLIENT
    CFLAGS += -DOC_CLIENT
endif

ifdef CONFIG_IOTIVITY_SERVER
   CFLAGS += -DOC_SERVER
endif

ifdef CONFIG_OC_DEBUG
    CFLAGS += -DOC_DEBUG
endif

ifdef CONFIG_APP_DEBUG
    CFLAGS += -DAPP_DEBUG
endif

ifdef CONFIG_DYNAMIC
    CFLAGS += -DOC_DYNAMIC_ALLOCATION
endif

ifdef CONFIG_SECURE
    CFLAGS += -DOC_SECURITY
endif

ifdef CONFIG_IPV4
    CFLAGS += -DOC_IPV4
endif

ifdef CONFIG_TCP
    CFLAGS += -DOC_TCP
endif
