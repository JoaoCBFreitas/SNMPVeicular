#
# Warning: you may need more libraries than are included here on the
# build line.  The agent frequently needs various libraries in order
# to compile pieces of it, but is OS dependent and we can't list all
# the combinations here.  Instead, look at the libraries that were
# used when linking the snmpd master agent and copy those to this
# file.
#

CC=gcc
TABLES=Tables
requestMonitoringDataTable=Tables/requestMonitoringDataTable
requestControlDataTable=Tables/requestControlDataTable
requestStatisticsDataTable=Tables/requestStatisticsDataTable
capabilitiesTable=Tables/capabilitiesTable
samplesTable=Tables/samplesTable
mapTypeTable=Tables/mapTypeTable
genericTypesTable=Tables/genericTypesTable
sampleUnitsTable=Tables/sampleUnitsTable
sampledValuesTable=Tables/sampledValuesTable
errorTable=Tables/errorTable
errorDescriptionTable=Tables/errorDescriptionTable
errorSensorTable=Tables/errorSensorTable





OBJ=obj


TARGETS=manager agent

CFLAGS=-I. `net-snmp-config --cflags`
BUILDLIBS=`net-snmp-config --libs`
BUILDAGENTLIBS=`net-snmp-config --agent-libs`

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

all: $(TARGETS)

agent:
	mkdir obj
	$(CC) -o $(OBJ)/auxFunc.o -c auxFunc.c -lm
	cd $(sampleUnitsTable) && $(MAKE) all && mv sampleUnitsTable.o ../../obj/
	cd $(capabilitiesTable) && $(MAKE) all && mv capabilitiesTable.o ../../obj/
	cd $(genericTypesTable) && $(MAKE) all && mv genericTypesTable.o ../../obj/
	cd $(mapTypeTable) && $(MAKE) all && mv mapTypeTable.o ../../obj/
	cd $(requestControlDataTable) && $(MAKE) all && mv requestControlDataTable.o ../../obj/
	cd $(requestMonitoringDataTable) && $(MAKE) all && mv requestMonitoringDataTable.o ../../obj/
	cd $(requestStatisticsDataTable) && $(MAKE) all && mv requestStatisticsDataTable.o ../../obj/
	cd $(samplesTable) && $(MAKE) all && mv samplesTable.o ../../obj/
	cd $(sampledValuesTable) && $(MAKE) all && mv sampledValuesTable.o ../../obj/
	cd $(errorTable) && $(MAKE) all && mv errorTable.o ../../obj/
	cd $(errorDescriptionTable) && $(MAKE) all && mv errorDescriptionTable.o ../../obj/
	cd $(errorSensorTable) && $(MAKE) all && mv errorSensorTable.o ../../obj/
	$(CC) $(CFLAGS) -o $(OBJ)/progAgent.o $(BUILDAGENTLIBS) -c agent.c
	$(CC) $(CFLAGS) -o agent $(OBJ)/*.o $(BUILDAGENTLIBS)
	rm -r obj
manager: 
	mkdir obj
	$(CC) $(CFLAGS) -o $(OBJ)/progManager.o $(BUILDLIBS) -c manager.c	
	$(CC) $(CFLAGS) -o manager $(OBJ)/progManager.o $(BUILDLIBS)
	rm -r obj
clean:
	rm $(TARGETS)
