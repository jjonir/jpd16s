CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lncurses -lrt
RM = rm -f
HARDWARE = lem1802 generic_clock m35fd
HARDWARE_OBJ = $(HARDWARE:%=%.o)
HARDWARE_MODULE_OBJ = $(HARDWARE:%=%_module.o)
# TODO don't do that ../foo thing
OBJECTS = core.o hardware_host.o dcpu16.o sim.o ../jpd16a/disasm.o $(HARDWARE_OBJ)
HARDWARE_MODULE = hardware_module_driver.o

.PHONY: all clean

all: jpd16s lem1802 generic_clock m35fd

jpd16s: $(OBJECTS)
	@echo LD $@
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

lem1802: $(HARDWARE_MODULE) lem1802_module.o
	@echo LD $@
	$(CC) $(LDFLAGS) -o $@ $(HARDWARE_MODULE) lem1802_module.o $(LDLIBS)

generic_clock: $(HARDWARE_MODULE) generic_clock_module.o
	@echo LD $@
	$(CC) $(LDFLAGS) -o $@ $(HARDWARE_MODULE) generic_clock_module.o $(LDLIBS)

m35fd: $(HARDWARE_MODULE) m35fd_module.o
	@echo LD $@
	$(CC) $(LDFLAGS) -o $@ $(HARDWARE_MODULE) m35fd_module.o $(LDLIBS)

%_module.o: %.c
	@echo CC $@
	$(CC) $(CFLAGS) -DBUILD_MODULE -c -o $@ $<

%.o: %.c
	@echo CC $@
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJECTS) $(HARDWARE_MODULE) $(HARDWARE_MODULE_OBJ) jpd16s $(HARDWARE)
