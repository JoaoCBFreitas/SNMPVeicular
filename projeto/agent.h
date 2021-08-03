//http://www.net-snmp.org/dev/agent/example_8h_source.html
/*
 * Don't include ourselves twice 
 */
#ifndef _OBUMIB_H
#define _OBUMIB_H
#endif
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/snmp_agent.h>
#include <net-snmp/agent/snmp_vars.h>
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/select.h>
#include <Tables/requestMonitoringDataTable/requestMonitoringDataTable.h>
#include <Tables/errorTable/errorTable.h>
#include <Tables/errorDescriptionTable/errorDescriptionTable.h>
#include <Tables/commandTable/commandTable.h>

#define DEMO_USE_SNMP_VERSION_3

#ifdef DEMO_USE_SNMP_VERSION_3
const char *our_v3_passphrase = "SNMPVeicular";
const char *our_v3_encryptionphrase = "SNMPV31cul4r";
#endif

const char *snmpusername = "snmpadmin";
/*
     * We use 'header_generic' from the util_funcs/header_generic module,
     *  so make sure this module is included in the agent.
     */
config_require(util_funcs / header_generic);
