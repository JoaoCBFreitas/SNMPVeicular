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

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include "Tables/errorDescriptionTable/errorDescriptionTable.h"
#include "Tables/sampleUnitsTable/sampleUnitsTable.h"
#include "Tables/genericTypesTable/genericTypesTable.h"
/*This struct will contain rules to decode a signal*/
typedef struct SG{
	char name[1024];
	int bitStart;
	int length;
	int endian;
	int signFlag;
	double scale;
	int offset;
	double min;
	double max;
	char unit[1024];
	char receiver[128];
	char description[10000];
}SG;
/*This struct will contain all signals decoding rules for a certain message*/
typedef struct SG_List{
	int capacity;
	int current;
	SG* list;
}SG_List;
/*This struct will contain all information regarding a certain CAN message*/
typedef struct BO{
	long messageID;
	char id[32];
	char name[1024];
	int length;
	char sender[128];
	char description[1024];
	SG_List* signals;
}BO;
/*This struct will contain all messages/signals and rules present in the dbc file*/
typedef struct BO_List{
	int current;
	int capacity;
	BO* list;
}BO_List;
/*This struc will store the decoded contents of a CAN message*/
typedef struct decodedCAN{
	char name[128];
	int signals;
	char signalname[100][32];
	double *value;
	char unit[100][32];
}decodedCAN;

/*This function removes all occurrences of a character from a string*/
char* removeChar(char* s,char c);
/*This function converts from decimal to hexadecimal*/
char* intToHex(int id);
/*This function converts from hexadecimal to decimal*/
int hexToInt(unsigned char* id);
/*This function applies the mask 1FFFFFF to the decimal ids present in .dbc files, returning their respective CAN ids*/
char* unMask(long id);
/*This function converts from decimal to an binary array*/
int* decToBinary(int n);
/*This function converts from a binary array to decimal*/
int binaryToDec(int* list,int n);
/*This function decodes the DATA present in a CAN frame according the the signal rules*/
int decodeData(unsigned char* data,int start,int length,int endian);
/*This function will decode a full CAN message and output a struct containing all relevant info*/
decodedCAN* decode(unsigned char* id, int dlc,unsigned char data[],BO_List* boList,decodedCAN* dc);
/*This function reads and adds the scale/offset from the CAN message to the SG struct*/
SG scaleBits(SG signal,char* token);
/*This function reads and adds the min/max from the CAN message to the SG struct*/
SG minMax(SG signal,char* token);
/*This function reads and adds the bitstart/length/endianess/signedness from the CAN message to the SG struct*/
SG signalBits(SG signal,char*token);
/*This function will alocate memory for a CAN message syntax rule*/
BO* getBO(char*line);
/*This function will alocate memory for all the CAN signal syntax rules for a CAN message*/
SG getSignal(char* line);
/*This function will read the dbc file and create all structs containg the decoding rules for CAN messages*/
BO_List* readDBC(char* file);
/*This function will read from the CAN interface and convert/convert CAN messages whose id matches the sensor given as input */
decodedCAN* parseCAN(char* sensor,BO_List* boList);

//sampleUnitsList *readSampleUnits(sampleUnitsList *,BO_List *);
int addToSampleUnits(sampleUnitsList *, long , char*);
errorDescrList *readErrorDescr(errorDescrList *);
//genericTypeList *readGenericTypes(genericTypeList *,BO_List *);
int addToGenericTypes(genericTypeList *,long , char*);