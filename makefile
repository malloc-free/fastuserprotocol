CC=gcc
DOC=doxygen
DOC_FILE=Doxyfile
CFLAGS=-c -fPIC -Wall -D_GNU_SOURCE `pkg-config --cflags glib-2.0`
INCLUDES=-Iincludes -includeudt.h -includeutp.h
LDFLAGS=-D_GNU_SOURCE 
LIBRARIES=-Llib -pthread -lutp -ludt -lgdsl -lrt -laio `pkg-config --libs glib-2.0`
SOURCES=src/tb_common.c src/tb_epoll.c src/tb_logging.c \
src/tb_file_io.c src/tb_sock_opt.c src/tb_worker.c src/tb_worker_pair.c \
src/tb_udt.c src/tb_udp.c src/tb_utp.c src/tb_stream.c src/tb_protocol.c src/tb_session.c \
src/tb_listener.c src/tb_endpoint.c src/tb_testbed.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=TestBed
SO_LIB=libtb.so.1.0
SO_FLAGS=-shared -Wl,-soname,libso.so.1

all: $(SOURCES) $(EXECUTABLE) $(SO_LIB) doxygen

executable: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBRARIES) -o $@

shared: $(SOURCES) $(SO_LIB)

$(SO_LIB) : $(OBJECTS)
	$(CC) $(SO_FLAGS) $(LDFLAGS) $(OBJECTS) $(LIBRARIES) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf src/*.o TestBed libtb.so.1.0 doc
doxygen:
	$(DOC) $(DOC_FILE)
install:
	export LD_LIBRARY_PATH=$(DIR):$$LD_LIBRARY_PATH
