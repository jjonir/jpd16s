CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lncurses
RM = rm -f
# TODO don't do that ../foo thing
OBJECTS = core.o sim.o ../jpd16a/disasm.o

.PHONY: all clean

all: jpd16s

jpd16s: $(OBJECTS)
	@echo LD $@
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	@echo CC $@
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJECTS) jpd16s
