CC = gcc
CFLAGS = -Wall -Wextra
RM = rm -f
OBJECTS = core.o sim.o

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
