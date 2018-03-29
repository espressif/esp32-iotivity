#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

export PROJECT_PATH := $(PWD)
export IDF_PATH ?= $(PWD)/esp-idf

include $(IDF_PATH)/make/project.mk

# sdkconfig is included project.mk recursively
# after setting sdkconfig done, start user layer macro define  
ifdef CONFIG_IOTIVITY_CLIENT
    PROJECT_NAME := iotivity_client
    CFLAGS += -DOC_CLIENT
endif

ifdef CONFIG_IOTIVITY_SERVER
    PROJECT_NAME := iotivity_server
   CFLAGS += -DOC_SERVER
endif

ifdef CONFIG_OC_DEBUG
    CFLAGS += -DOC_DEBUG
endif

ifdef CONFIG_APP_DEBUG
    CFLAGS += -DAPP_DEBUG
endif