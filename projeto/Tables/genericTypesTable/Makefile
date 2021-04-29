TARGETS= genericTypesTable.o


CFLAGS=-I. `net-snmp-config --cflags`
BUILDLIBS=`net-snmp-config --libs net-snmp-config --external-libs`
BUILDAGENTLIBS=`net-snmp-config --agent-libs`

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

all: $(TARGETS)

genericTypesTable.o: genericTypesTable.c Makefile
		$(CC) $(CFLAGS) $(DLFLAGS) -c -o genericTypesTable.o genericTypesTable.c
#		$(CC) $(CFLAGS) $(DLFLAGS) -o genericTypesTable.so genericTypesTable.o
