TARGETS=commandTable.o


CFLAGS=-I. `net-snmp-config --cflags`
BUILDLIBS=`net-snmp-config --libs net-snmp-config --external-libs`
BUILDAGENTLIBS=`net-snmp-config --agent-libs`

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

all: $(TARGETS)


commandTable.o: commandTable.c commandTable.h Makefile
		$(CC) $(CFLAGS) $(DLFLAGS) -c -o commandTable.o commandTable.c
