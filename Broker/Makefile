TARGET = Broker
LIBS = -lcommons -lSerializacion -lpthread
CC = gcc
CFLAGS = -g -Wall -L$(Serializacion_PATH) -Wl,-rpath=$(Serializacion_PATH)

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@
	
.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@
	
clean:
	-rm -f *.o
	-rm -f $(TARGET)