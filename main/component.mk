#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

#CFLAGS += -DOC_SERVER

ifdef CONFIG_IOTIVITY_CLIENT
	COMPONENT_SRCDIRS += esp32_client
    CFLAGS += -DOC_CLIENT
endif

ifdef CONFIG_IOTIVITY_SERVER
	COMPONENT_SRCDIRS += esp32_server
    CFLAGS += -DOC_SERVER
endif

COMPONENT_SRCDIRS += esp32_lightbulb

COMPONENT_ADD_INCLUDEDIRS := esp32_lightbulb
