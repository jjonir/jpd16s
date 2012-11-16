CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lncurses
RM = rm -f
HARDWARE_OBJ = lem1802.o generic_clock.o m35fd.o
# TODO don't do that ../foo thing
OBJECTS = core.o hardware_hardwire.o interrupts.o sim.o ../jpd16a/disasm.o $(HARDWARE_OBJ)

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
