TARGETS=sampleUnitsTable.o


CFLAGS=-I. `net-snmp-config --cflags`
BUILDLIBS=`net-snmp-config --libs net-snmp-config --external-libs`
BUILDAGENTLIBS=`net-snmp-config --agent-libs`

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

all: $(TARGETS)


sampleUnitsTable.o: sampleUnitsTable.c Makefile
		$(CC) $(CFLAGS) $(DLFLAGS) -c -o sampleUnitsTable.o sampleUnitsTable.c
#		$(CC) $(CFLAGS) $(DLFLAGS) -o sampleUnitsTable.so sampleUnitsTable.o
