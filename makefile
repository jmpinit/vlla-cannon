PROG := vlla-cannon

SRCDIR := src
OBJDIR := obj
BINDIR := bin

SOURCES = vlla-cannon.c \
	  kiss_fft.c \
	  kiss_fftr.c \
	  audio.c

INCLUDES = -Isrc/inc/common -Isrc/inc -I/usr/local/include
OBJECTS = $(patsubst %,$(OBJDIR)/%,$(SOURCES:.c=.o))

CFLAGS := -DRPI_NO_X -g -std=c99
LFLAGS = -lm -L/usr/local/lib -L./src/common -lasound -pthread -lvlla
CC := gcc

all: $(PROG)

run: $(PROG)
	bin/$(PROG)
	
debug: $(PROG)
	gdb bin/$(PROG)

# linking the program.
$(PROG): $(OBJECTS) $(BINDIR)
	$(CC) $(OBJECTS) -o $(BINDIR)/$(PROG) $(LFLAGS)

$(BINDIR):
	mkdir -p $(BINDIR)

# compiling source files.
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c -s -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)/common

clean:
ifeq ($(OS),Windows_NT)
	del $(OBJECTS)
else
	rm $(OBJECTS)
endif

.PHONY: all clean

