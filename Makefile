WARNINGS = -Wall -Wextra -Wpedantic -Werror
WARNINGS += -Wcast-align
WARNINGS += -Wformat=2
WARNINGS += -Wlogical-op
WARNINGS += -Wmissing-include-dirs
WARNINGS += -Wnested-externs
WARNINGS += -Wdisabled-optimization
WARNINGS += -Wunsafe-loop-optimizations
WARNINGS += -Wfree-nonheap-object
# Ignored errors
WARNINGS += -Wno-missing-field-initializers

CC = gcc
CFLAGS = -std=c89 -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lX11
SOURCES = *.c
EXEC = dael

.PHONY: all install clean

all: build

build:
	$(CC) -g $(CFLAGS) $(WARNINGS) $(SOURCES) $(LDFLAGS) -o $(EXEC)
	#$(CC) -g $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $(EXEC)

run:
	./$(EXEC)

install: all
	install -m 755 $(EXEC) /usr/bin

clean:
	-rm $(EXEC)
