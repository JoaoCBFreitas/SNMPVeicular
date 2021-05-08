#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>
typedef struct SG{
	char name[32];
	int bitStart;
	int length;
	int endian;
	int signFlag;
	double scale;
	int offset;
	double min;
	double max;
	char unit[10];
	char receiver[128];
}SG;
typedef struct SG_List{
	int capacity;
	int current;
	SG* list;
}SG_List;
typedef struct BO{
	char id[32];
	char name[128];
	int length;
	char sender[128];
	SG_List* signals;
}BO;
typedef struct BO_List{
	int current;
	int capacity;
	BO* list;
}BO_List;

typedef struct decodedCAN{
	char *id;
	int *value;
	char* unit;
}decodedCAN;
char* removeChar(char* s,char c){
	int tam=strlen(s);
	int j;
	for(int i=j=0;i<tam;i++)
		if(s[i]!=c)
			s[j++]=s[i];
	s[j]='\0';
	return s;
}
char* intToHex(int id){
	unsigned char* hex=malloc(sizeof(char)*8+1);
	sprintf(hex,"%X",id);
	return hex;
}
int hexToInt(unsigned char* id){
	long long decimal=0;
	int i=0,val,len;
	len=strlen(id);
	len--;
	for(i=0;id[i]!='\0';i++){
		if(id[i]>='0'&& id[i]<='9'){
			val=id[i]-48;
		}else if(id[i]>='A'&& id[i]<='F'){
			val=id[i]-65+10;
		}
		decimal+=val*pow(16,len);
		len--;
	}
	return decimal;
}
char* unMask(long id){
	u_int32_t unOrig=(u_int32_t) id;
	u_int32_t mask=0x1FFFFFFF;
	u_int32_t fin=unOrig & mask;
	char *res=intToHex((int)fin);
	return res;
}
decodedCAN* decode(unsigned char* id, int dlc,unsigned char* data){
	decodedCAN* dc=(decodedCAN*)malloc(sizeof(decodedCAN));
	if(strlen(id)==3){
		/*11-bit identifier*/
		/*char*res=searchID(hexToInt(id));*/
		//strcpy(dc->id,res);
	}else{
		/*29-bit identifier*/
		/**/
	}
	return dc;
}
void scaleBits(SG* signal, char* token){
	SG res=*signal;
	char *aux=malloc(sizeof(char)*strlen(token)+1);
	int i;

	strcpy(aux,token);
	aux++;
	aux[strlen(aux)-1]='\0';
	for(i=0;aux[i]!=',';i++);
	char s[128];
	strncpy(s,aux,i);
	s[i]='\0';
	res.scale=atof(s);
	aux+=i+1;
	res.offset=atof(aux);
}
void minMax(SG* signal, char*token){
	SG res=*signal;
	char *aux=malloc(sizeof(char)*strlen(token)+1);
	strcpy(aux,token);
	int i;

	aux++;
	aux[strlen(aux)-1]='\0';
	for(i=0;aux[i]!='|';i++);
	char s[128];
	strncpy(s,aux,i);
	s[i]='\0';
	res.min=atof(s);
	aux+=i+1;
	res.max=atof(aux);
}
void signalBits(SG* signal,char* token){
	SG res=*signal;
	char *aux=malloc(sizeof(char)*strlen(token)+1);
	strcpy(aux,token);
	int j;

	for(j=0;aux[j]!='|';j++);
	char bitS[j+1];
	strncpy(bitS,aux,j);
	bitS[j]='\0';
	res.bitStart=atoi(bitS);
	aux+=j+1;
	/*String after the | character*/
	for(j=0;aux[j]!='@';j++);
	char len[j+1];
	strncpy(len,aux,j);
	len[j]='\0';
	res.length=atoi(len);
	aux+=j+1;
	/*String after the @*/
	res.endian=aux[0]-'0';
	if(aux[1]=='+')
		res.signFlag=1;
	else
		res.signFlag=0;
}
BO* getBO(char* line){
	BO* res=(BO*)malloc(sizeof(BO));
	memset(res,0,sizeof(BO*));
	strtok(line, "\n");
	char* token=strtok(line," ");
	int j=0;
	long idLine;
	while(token!=NULL){
		switch(j){
			case 0:
				break;
			case 1:
				idLine=atol(token);
				/*Decode ID*/
				char *decodedID=unMask(idLine);
				strcpy(res->id,decodedID);
				break;
			case 2:
				strcpy(res->name,token);
				break;
			case 3:
				res->length=atoi(token);
				break;
			case 4:
				strcpy(res->sender,token);
				break;
			default:
				break;
		}
		token=strtok(NULL," ");
		j++;
	}
	res->signals=(SG_List*)malloc(sizeof(SG_List));
	res->signals->list=(SG*) malloc(sizeof(SG));
	res->signals->capacity=1;
	res->signals->current=0;
	return res;
}
SG getSignal(char* line){
	SG res;
	strtok(line, "\n");
	char* token=strtok(line," ");
	int j=0;
	char s[1000];
	char s1[1000];
	char* unit="";
	while(token!=NULL){
		switch(j){
			case 0:
				break;
			case 1:
				strcpy(res.name,token);
				break;
			case 2:
				if(strcmp(token,":")!=0){
					strcpy(s,res.name);
					strcpy(s1,"");
					strcat(s1,s);
					strcat(s1," ");
					strcat(s1,token);
					strcpy(res.name,s1);
					token=strtok(NULL," ");
				}
				break;
			case 3:
				//22|2@1+
				signalBits(&res,token);
				break;
			case 4:
				//(1,0)
				scaleBits(&res,token);
				break;
			case 5:
				//[0|3]
				minMax(&res,token);
				break;
			case 6:
				//"ms"
				if(strcmp(token,"")!=0)
					unit= removeChar(token,'\"');
				strcpy(res.unit,unit);
				break;
			case 7:
				//Vector__XXX
				strcpy(res.receiver,token);
				break;
			default:
				break;
		}
		token=strtok(NULL," ");
		j++;
	}
	return res;
}
BO_List* readDBC(char* file){
	BO_List* boList = (BO_List *)malloc(sizeof(BO_List));
    boList->capacity = 1;
    boList->current = 0;
    boList->list =(BO*)malloc(sizeof(BO));
    FILE *fp;
    char *line = malloc(sizeof(char)*100000);
    size_t len = 0;
    ssize_t read;
    fp = fopen(file, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    while ((read = getline(&line, &len, fp)) != -1)
    {
		if(strlen(line)==0) continue;
		char bo_[5]="";
		strncpy(bo_,line,4);
		bo_[5]='\0';
		if(strcmp(bo_,"BO_ ")==0){
			BO* bo=getBO(line);
			if (boList->capacity == boList->current)
			{
				BO_List *boList2 = (BO_List*)malloc(sizeof(BO_List));
				boList2->capacity = boList->capacity * 2;
				boList2->current = boList->current;
				boList2->list = malloc(sizeof(BO) * boList2->capacity);
				for (int j = 0; j < boList2->current; j++)
				{
					boList2->list[j] = boList->list[j];
				}
				boList->capacity = boList2->capacity;
				boList->current = boList2->current;
				boList->list=boList2->list;
				free(boList2);
			}
			boList->list[boList->current] = *bo;
			boList->current++;
			free(bo);
		}else if(strcmp(bo_," SG_")==0){ 
			int lastInserted=boList->current-1;
			int nSignals=boList->list[lastInserted].signals->current;
			SG signal=getSignal(line);
			if(nSignals==boList->list[lastInserted].signals->capacity){
				SG_List sgList;
				sgList.capacity = boList->list[lastInserted].signals->capacity * 2;
				sgList.current = boList->list[lastInserted].signals->current;
				sgList.list = malloc(sizeof(SG) * sgList.capacity);
				
				for (int j = 0; j < sgList.current; j++)
				{
					sgList.list[j]=boList->list[lastInserted].signals->list[j];
				}
				boList->list[lastInserted].signals->capacity = sgList.capacity;
				boList->list[lastInserted].signals->current = sgList.current;
				boList->list[lastInserted].signals->list = sgList.list;
			}
			boList->list[lastInserted].signals->list[nSignals]=signal;
			boList->list[lastInserted].signals->current++;
			
		}
    }
    fclose(fp);
    free(line);
    return boList;
}

int main(int argc, char **argv)
{
	/*Read dbc file and populate the structs above*/
	BO_List* boList=readDBC("J1939/J1939.dbc");
	int s, i; 
	int nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket");
		return 1;
	}

	strcpy(ifr.ifr_name, "vcan0" );
	ioctl(s, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind");
		return 1;
	}
    while(1){
        nbytes = read(s, &frame, sizeof(struct can_frame));

        if (nbytes < 0) {
            perror("Read");
            return 1;
        }
        printf("0x%08X [%d] ",frame.can_id, frame.can_dlc);
		unsigned char data[frame.can_dlc*2];
		/*frame.data is inverted for some reason*/
		for (int j=0,i = frame.can_dlc-1; i >=0; i--,j++){
			data[j]=frame.data[i];
		}
		//decodedCAN *dc=decode(frame.can_id,frame.can_dlc,data);
	    for (i = 0;i<frame.can_dlc-1; i++)
		    printf("%02X ",data[i]);

	    printf("\r\n");
    }

	if (close(s) < 0) {
		perror("Close");
		return 1;
	}

	return 0;
}