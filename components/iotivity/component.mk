#
# Component Makefile
#

CFLAGS += -DOC_CLIENT
#CFLAGS += -DOC_SERVER

COMPONENT_ADD_INCLUDEDIRS :=  \
adapter	\
iotivity-constrained	\
iotivity-constrained/include \
iotivity-constrained/messaging/coap \
iotivity-constrained/port 	\
iotivity-constrained/util	\
iotivity-constrained/util/pt   \
iotivity-constrained/deps/tinycbor/src

COMPONENT_OBJS =  \
iotivity-constrained/deps/tinycbor/src/cborencoder.o	\
iotivity-constrained/deps/tinycbor/src/cborencoder_close_container_checked.o	\
iotivity-constrained/deps/tinycbor/src/cborparser.o	\
\
adapter/random.o	\
adapter/storage.o	\
adapter/clock.o		\
adapter/ipadapter.o	\
adapter/ipadapter.o	\
adapter/abort.o		\
\
iotivity-constrained/util/oc_etimer.o \
iotivity-constrained/util/oc_list.o \
iotivity-constrained/util/oc_memb.o \
iotivity-constrained/util/oc_mmem.o \
iotivity-constrained/util/oc_process.o \
iotivity-constrained/util/oc_timer.o \
\
iotivity-constrained/api/oc_base64.o \
iotivity-constrained/api/oc_blockwise.o \
iotivity-constrained/api/oc_buffer.o \
iotivity-constrained/api/oc_client_api.o \
iotivity-constrained/api/oc_collection.o \
iotivity-constrained/api/oc_core_res.o \
iotivity-constrained/api/oc_discovery.o \
iotivity-constrained/api/oc_helpers.o \
iotivity-constrained/api/oc_main.o \
iotivity-constrained/api/oc_network_events.o \
iotivity-constrained/api/oc_rep.o \
iotivity-constrained/api/oc_ri.o \
iotivity-constrained/api/oc_server_api.o \
iotivity-constrained/api/oc_uuid.o \
\
iotivity-constrained/messaging/coap/coap.o	\
iotivity-constrained/messaging/coap/engine.o	\
iotivity-constrained/messaging/coap/observe.o	\
iotivity-constrained/messaging/coap/separate.o	\
iotivity-constrained/messaging/coap/transactions.o	



COMPONENT_SRCDIRS :=  \
iotivity-constrained/util  \
iotivity-constrained/api \
iotivity-constrained/messaging/coap	\
iotivity-constrained/deps/tinycbor/src	\
adapter














