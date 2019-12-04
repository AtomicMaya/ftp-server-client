CC=gcc
CCFLAGS=-Wall -g
LDFLAGS=
COMPNAME=maya_b_ftp

SOURCES_COMMON=$(wildcard common/*.c)

SOURCES_CLIENT=$(SOURCES_COMMON) $(wildcard client/*.c)
TARGET_CLIENT=ftp-client
OBJECTS_CLIENT=$(SOURCES_CLIENT:.c=.o) 

SOURCES_SERVER=$(SOURCES_COMMON) $(wildcard server/*.c)
TARGET_SERVER=ftp-server
OBJECTS_SERVER=$(SOURCES_SERVER:.c=.o)

SOURCES_LSERVER=$(SOURCES_COMMON) $(wildcard light_server/*.c)
TARGET_LSERVER=ftp-lserver
OBJECTS_LSERVER=$(SOURCES_LSERVER:.c=.o)

all: $(TARGET_CLIENT) $(TARGET_SERVER) $(TARGET_LSERVER)

$(TARGET_CLIENT): $(OBJECTS_CLIENT)
		$(CC) -o client/$@ $^ $(LDFLAGS)

$(TARGET_SERVER): $(OBJECTS_SERVER)
		$(CC) -o server/$@ $^ $(LDFLAGS)

$(TARGET_LSERVER): $(OBJECTS_LSERVER)
		$(CC) -o light_server/$@ $^ $(LDFLAGS)

*/%.o: %.c %.h
		$(CC) $(CCFLAGS) -c $<

*/%.o: %.c
		$(CC) $(CCFLAGS) -c $<

clean:
		rm -f */*.o client/$(TARGET_CLIENT) server/$(TARGET_SERVER) light_server/$(TARGET_LSERVER)

wc: 
		wc -l */*.c */*.h

zip:
		zip $(COMPNAME).zip */*.c */*.h Makefile README.md

delzip:
		rm $(COMPNAME).zip

sha:
		sha256sum client/*.h client/*.c common/*.h common/*.c server/*.h server/*.c light_server/*.h light_server/*.c