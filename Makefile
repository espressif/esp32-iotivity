#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

ifdef CONFIG_IOTIVITY_CLIENT
	PROJECT_NAME := iotivity_client
else
	PROJECT_NAME := iotivity_server
endif

include $(IDF_PATH)/make/project.mk
