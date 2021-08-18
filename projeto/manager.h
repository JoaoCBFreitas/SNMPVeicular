#include "managerFunc.h"
#define NETSNMP_DS_WALK_INCLUDE_REQUESTED 1
#define NETSNMP_DS_WALK_PRINT_STATISTICS 2
#define NETSNMP_DS_WALK_DONT_CHECK_LEXICOGRAPHIC 3
#define DEMO_USE_SNMP_VERSION_3

#ifdef DEMO_USE_SNMP_VERSION_3
const char *our_v3_passphrase = "SNMPVeicular";
const char *our_v3_encryptionphrase = "SNMPV31cul4r";
#endif

const char *snmpusername = "snmpadmin";
