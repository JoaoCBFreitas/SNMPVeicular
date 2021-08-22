#ifndef MANAGERFUNC_H
#define MANAGERFUNC_H
#endif
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
/*
  Table OIDs
*/
static char *requestMonitoringDataTable = ".1.3.6.1.3.8888.1";
static oid requestMonitoringDataTableOid[] = {1, 3, 6, 1, 3, 8888, 1};

static char *requestControlDataTable = ".1.3.6.1.3.8888.2";
static oid requestControlDataTableOid[] = {1, 3, 6, 1, 3, 8888, 2};

static char *requestStatisticsDataTable = ".1.3.6.1.3.8888.3";
static oid requestStatisticsDataTableOid[] = {1, 3, 6, 1, 3, 8888, 3};

static char *capabilitiesTable = ".1.3.6.1.3.8888.4";
static oid capabilitiesTableOid[] = {1, 3, 6, 1, 3, 8888, 4};

static char *samplesTable = ".1.3.6.1.3.8888.5";
static oid samplesTableOid[] = {1, 3, 6, 1, 3, 8888, 5};

static char *mapTypeTable = ".1.3.6.1.3.8888.6";
static oid mapTypeTableOid[] = {1, 3, 6, 1, 3, 8888, 6};

static char *genericTypesTable = ".1.3.6.1.3.8888.7";
static oid genericTypesTableOid[] = {1, 3, 6, 1, 3, 8888, 7};

static char *sampleUnitsTable = ".1.3.6.1.3.8888.8";
static oid sampleUnitsTableOid[] = {1, 3, 6, 1, 3, 8888, 8};

static char *errorTable = ".1.3.6.1.3.8888.9";
static oid errorTableOid[] = {1, 3, 6, 1, 3, 8888, 9};

static char *errorDescriptionTable = ".1.3.6.1.3.8888.10";
static oid errorDescriptionTableOid[] = {1, 3, 6, 1, 3, 8888, 10};

static char *commandTemplateTable = ".1.3.6.1.3.8888.11";
static oid commandTemplateTableOid[] = {1, 3, 6, 1, 3, 8888, 11};

static char *commandTable = ".1.3.6.1.3.8888.12";
static oid commandTableOid[] = {1, 3, 6, 1, 3, 8888, 12};

#define CommandNumber 3
#define CommandOid 10
static oid oidListCommand[CommandNumber][CommandOid] = {{1, 3, 6, 1, 3, 8888, 12, 1, 2, 0}, {1, 3, 6, 1, 3, 8888, 12, 1, 3, 0}, {1, 3, 6, 1, 3, 8888, 12, 1, 4, 0}};
static char *oidStringCommand[] = {"TemplateID.", "CommandInput.", "CommandUser."};
static char *typesCommand = "uis";

#define RequestNumber 10
#define RequestOid 10
static oid oidListRequest[RequestNumber][RequestOid] = {{1, 3, 6, 1, 3, 8888, 1, 1, 3, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 4, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 5, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 8, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 10, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 11, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 12, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 15, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 16, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 18, 0}};
static char *oidStringRequest[] = {"RequestMapID.", "RequestStatisticsID.", "SavingMode.", "StartTime.", "WaitTime.", "DurationTime.", "ExpireTime.", "MaxNOfSamples.", "LoopMode.", "RequestUser."};
static char *typesRequest = "uuissssuis";

#define EditNumber 4
#define EditOid 10
static oid oidListEditRequest[EditNumber][EditOid] = {{1, 3, 6, 1, 3, 8888, 1, 1, 5, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 15, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 16, 0}, {1, 3, 6, 1, 3, 8888, 1, 1, 17, 0}};
static char *oidStringEditRequest[] = {"SavingMode.", "MaxNOfSamples.", "LoopMode.", "Status."};
static char *typesEditRequest = "uuuu";

typedef struct table_contents
{
  struct table_contents *next;
  netsnmp_variable_list *data;
} table_contents;

typedef struct active_errors
{
  struct active_errors *next;
  int id;
  char *index;
  int indexError;
  char *timestamp;
  char *errorDesc;
  char *errorCode;
  char *username;
} active_errors;

typedef struct command_template
{
  struct command_template *next;
  int id;
  char *descr;
  char *target;
} command_template;
/*This function will, based on user input, send bulkget requests to the agent so as to obtain table contents*/
void viewTables(netsnmp_session session, netsnmp_session *ss);
/*This function will display all currently active errors in the system*/
void activeErrors(netsnmp_session session, netsnmp_session *ss);
/*This function send a new command into the system*/
void sendCommand(netsnmp_session session, netsnmp_session *ss);
/*This function send a new request into the system*/
void sendRequest(netsnmp_session session, netsnmp_session *ss);