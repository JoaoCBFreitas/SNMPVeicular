#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/snmp_agent.h>
#include <net-snmp/agent/snmp_vars.h>

#include "Tables/errorDescriptionTable/errorDescriptionTable.h"
#include "Tables/sampleUnitsTable/sampleUnitsTable.h"
#include "Tables/genericTypesTable/genericTypesTable.h"
/*This function will add a sample unit to a list of sample units, 
  returning the index in which the sample unit was inserted, 
  if the sample unit is already in the list it will return its index*/
int addToSampleUnits(sampleUnitsList *, long, char *);
/*This function will add a generic type to a list of generic types, 
  returning the index in which the generic type was inserted, 
  if the generic type is already in the list it will return its index*/
int addToGenericTypes(genericTypeList *, long, char *);
/*This function will return a 32bit integer checksum encoded as an string containing its hex values, courtesy of user schnaader at stackoverflow
  It's a weak checksum however it should serve it's purpose, allowing identification of values obtained from the same message, but different signals.
  It will use the timestamp and message name to create the checksum
*/
char *createChecksum(char *);