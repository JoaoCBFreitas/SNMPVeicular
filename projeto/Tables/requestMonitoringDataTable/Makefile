TARGETS=requestMonitoringDataTable.o


CFLAGS=-I. `net-snmp-config --cflags`
BUILDLIBS=`net-snmp-config --libs net-snmp-config --external-libs`
BUILDAGENTLIBS=`net-snmp-config --agent-libs`

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

all: $(TARGETS)


requestMonitoringDataTable.o: requestMonitoringDataTable.c requestMonitoringDataTable.h Makefile
		$(CC) $(CFLAGS) $(DLFLAGS) -c -o requestMonitoringDataTable.o requestMonitoringDataTable.c
