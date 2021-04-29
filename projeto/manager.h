#include <net-snmp/net-snmp-config.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/types.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <stdio.h>
#include <ctype.h>
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <net-snmp/net-snmp-includes.h>
#define NETSNMP_DS_WALK_INCLUDE_REQUESTED 1
#define NETSNMP_DS_WALK_PRINT_STATISTICS 2
#define NETSNMP_DS_WALK_DONT_CHECK_LEXICOGRAPHIC 3
#define DEMO_USE_SNMP_VERSION_3

#ifdef DEMO_USE_SNMP_VERSION_3
const char *our_v3_passphrase = "SNMPVeicular";
const char *our_v3_encryptionphrase = "SNMPV31cul4r";
#endif

const char *snmpusername = "snmpadmin";
/*
  Table OIDs
*/
char *requestMonitoringDataTable = ".1.3.6.1.3.8888.1";
oid requestMonitoringDataTableOid[] = {1, 3, 6, 1, 3, 8888, 1};

char *requestControlDataTable = ".1.3.6.1.3.8888.2";
oid requestControlDataTableOid[] = {1, 3, 6, 1, 3, 8888, 2};

char *requestStatisticsDataTable = ".1.3.6.1.3.8888.3";
oid requestStatisticsDataTableOid[] = {1, 3, 6, 1, 3, 8888, 3};

char *capabilitiesTable = ".1.3.6.1.3.8888.4";
oid capabilitiesTableOid[] = {1, 3, 6, 1, 3, 8888, 4};

char *samplesTable = ".1.3.6.1.3.8888.5";
oid samplesTableOid[] = {1, 3, 6, 1, 3, 8888, 5};

char *mapTypeTable = ".1.3.6.1.3.8888.6";
oid mapTypeTableOid[] = {1, 3, 6, 1, 3, 8888, 6};

char *genericTypesTable = ".1.3.6.1.3.8888.7";
oid genericTypesTableOid[] = {1, 3, 6, 1, 3, 8888, 7};

char *sampleUnitsTable = ".1.3.6.1.3.8888.8";
oid sampleUnitsTableOid[] = {1, 3, 6, 1, 3, 8888, 8};

char *sampledValuesTable = ".1.3.6.1.3.8888.9";
oid sampledValuesTableOid[] = {1, 3, 6, 1, 3, 8888, 9};

char *errorTable = ".1.3.6.1.3.8888.10";
oid errorTableOid[] = {1, 3, 6, 1, 3, 8888, 10};

char *errorDescriptionTable = ".1.3.6.1.3.8888.11";
oid errorDescriptionTableOid[] = {1, 3, 6, 1, 3, 8888, 11};

char *errorSensorTable = ".1.3.6.1.3.8888.12";
oid errorSensorTableOid[] = {1, 3, 6, 1, 3, 8888, 12};

char *moduleTable = ".1.3.6.1.3.8888.13";
oid moduleTableOid[] = {1, 3, 6, 1, 3, 8888, 13};

char *moduleDescriptionTable = ".1.3.6.1.3.8888.14";
oid moduleDescriptionTableOid[] = {1, 3, 6, 1, 3, 8888, 14};

char *componentTable = ".1.3.6.1.3.8888.15";
oid componentTableOid[] = {1, 3, 6, 1, 3, 8888, 15};

struct host
{
  char *name;
  char *community;
} hosts[] = {
    {"test1", "public"},
    {"test2", "public"},
    {"test3", "public"},
    {"test4", "public"},
    {NULL}};

/*
  * a list of variables to query for
  */
struct oid
{
  char *Name;
  oid anOid[MAX_OID_LEN];
  int OidLen;
} oids[] = {
    {"mapTypeTable"},
    {"interfaces.ifNumber.1"},
    {"interfaces.ifNumber.0"},
    {NULL}};