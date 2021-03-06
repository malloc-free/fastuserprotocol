CC=gcc
DOC=doxygen
DOC_FILE=Doxyfile
METRICS=cccc
METRICS_FILE=metrics.html
CFLAGS=-c -fPIC -Wall -D_GNU_SOURCE
INCLUDES=-Iincludes -includeudt.h -includeutp.h
LDFLAGS=-D_GNU_SOURCE 
LIBRARIES=-Llib -pthread -lutp -ludt -lrt 
SOURCES=src/tb_common.c src/tb_epoll.c src/tb_logging.c src/tb_sock_opt.c \
src/tb_udt.c src/tb_udp.c src/tb_utp.c src/tb_stream.c src/tb_protocol.c \
src/tb_session.c src/tb_listener.c src/tb_endpoint.c src/tb_testbed.c
HEADERS=src/*.h
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
	rm -rf src/*.o TestBed libtb.so.1.0 doc metrics.html .cccc
doxygen:
	$(DOC) $(DOC_FILE)
metrics:
	$(METRICS) $(SOURCES) $(HEADERS) --html_outfile=$(METRICS_FILE) 
install:
	export LD_LIBRARY_PATH=$(DIR):$$LD_LIBRARY_PATH
