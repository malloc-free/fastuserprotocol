CC=gcc
CFLAGS=-Wall -D_GNU_SOURCE
INCLUDE_DIR=includes/
INCLUDES=utp.h udt.h utypes.h
INCLUDE_FILES := $(foreach inc, $(INCLUDES), $(addprefix $(INCLUDE_DIR), int))
LDFLAGS=-pthread -ludt -lutp -lgdsl -lrt -laio
LIB_DIR=lib
EXECUTABLE=TestBed
SOURCES=tb_common.c tb_epoll.c tb_file_io.c tb_listener.c \
tb_logging.c tb_protocol.c tb_server.c tb_session.c tb_sock_opt.c \
tb_testbed.c tb_udp.c tb_utp.c tb_worker_pair.c tb_worker.c
SRC_DIR=src/
SOURCE_FILES := $(foreach src, $(SOURCES), $(addprefix $(SRC_DIR), src))
OBJECTS=$(SOURCES:.c=.o)
all: $(addsuffx $(SOURCE_FILES) $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIB_DIR) $(LDFLAGS) -o $@

tb_testbed.o:
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $(foreach i_file, $(INCLUDE_FILES) , $(addprefix -include
\i_file)) $< -o $@
tb_common.o:
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $(-includeINCLUDE_FILES)
