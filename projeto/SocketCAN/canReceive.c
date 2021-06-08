#include "canReceive.h"

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
	sprintf(hex,"%08X",id);
	return hex;
}
int hexToInt(unsigned char* id){
	long long decimal=0;
	char* aux=malloc(8*sizeof(char));
	sprintf(aux,"%X",id[0]);
	int i=0,val,len;
	len=strlen(aux);
	len--;
	for(i=0;aux[i]!='\0';i++){
		if(aux[i]>='0'&& aux[i]<='9'){
			val=aux[i]-48;
		}else if(aux[i]>='A'&& aux[i]<='F'){
			val=aux[i]-65+10;
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
int* decToBinary(int n){
    static int binaryNum[4];
    int i=7;
	if(n==0){
		for(int j=0;j<8;j++){
			binaryNum[j]=0;
		}
	}else{
		while(n>0){
			binaryNum[i]=n%2;
			n=n/2;
			i--;
    	}
	}
    return binaryNum;
}
int binaryToDec(int* list,int n){
	int res=0;
	int multiplier=1;
	for(int i=0;i<n;i++){
		res+=(multiplier*list[n-1-i]);
		multiplier*=2;
	}	
	return res;
}
int decodeData(int* binData,int start,int length,int endian){
    int binRes[length];
    //Get relevant bits
    for(int i=0;i<length;i++){
        binRes[i]=binData[start+i];
    }
    int decFinal=binaryToDec(binRes,length);
    char* hexFinal=intToHex(decFinal);
    //Remove less significative bits 00056=56
	int tam=strlen(hexFinal);
    for(int i=0;i<strlen(hexFinal);i++){
        if(hexFinal[i]=='0'){
            hexFinal++;
            i--;
			tam--;
        }else{
            break;
        }
    }
    if(endian==1){
        char aux[tam];
        for(int i=tam-1,k=0;i>=0;i-=2,k+=2){
            aux[k]=hexFinal[i-1];
            aux[k+1]=hexFinal[i];
        }
        strcpy(hexFinal,aux);
        
    }
	hexFinal[tam]='\0';
	return (int) strtoul(hexFinal,NULL,16);
}
void hexToBinary(int* binData,unsigned char* data){
    int cont=0;
    //Convert from char* Hex to int* bit array
    for(int i=0;i<8;i++){
        unsigned char *aux=malloc(sizeof(char));
        *aux=data[i];
        int a=hexToInt(aux);
        free(aux);
        int* b=decToBinary(a);
        for(int j=0;j<8;j++){
            binData[cont]=b[j];
            cont++;
        }
    }
}
decodedCAN* decode(unsigned char* id, int dlc,unsigned char data[],BO_List* boList,decodedCAN* dc){
	int *binData=malloc(sizeof(int)*64);
	hexToBinary(binData,data);
	dc->signals=0;
	if(strlen(id)==3){
		/*11-bit identifier*/
		/*char*res=searchID(hexToInt(id));*/
		//strcpy(dc->id,res);
	}else{
		/*29-bit identifier*/
		for(int i=0;i<boList->current;i++){
			if(strcmp(boList->list[i].id,id)==0){
				strcpy(dc->name,boList->list[i].name);
				dc->name[strlen(boList->list[i].name)]='\0';
				dc->value=malloc(sizeof(int)*boList->list[i].signals->current);
				dc->signals=boList->list[i].signals->current;
				for(int k=0;k<boList->list[i].signals->current;k++){
					strcpy(dc->signalname[k],boList->list[i].signals->list[k].name);
					
					/*based of start bit and bit length, obtain relevant data from data PDU*/
					int value=decodeData(binData,boList->list[i].signals->list[k].bitStart,boList->list[i].signals->list[k].length,boList->list[i].signals->list[k].endian);
					double decodedValue=boList->list[i].signals->list[k].offset+boList->list[i].signals->list[k].scale*value;
					if(decodedValue>boList->list[i].signals->list[k].max)
						decodedValue-=boList->list[i].signals->list[k].min;
					if(decodedValue<boList->list[i].signals->list[k].min)
						decodedValue+=boList->list[i].signals->list[k].max;
					dc->value[k]=decodedValue;
					strcpy(dc->unit[k],boList->list[i].signals->list[k].unit);
				}
				break;
			}
		}
	}
	free(binData);
	return dc;
}
SG scaleBits(SG signal, char* token){
	char *aux=malloc(sizeof(char)*strlen(token)+1);
	int i;

	strcpy(aux,token);
	aux++;
	aux[strlen(aux)-1]='\0';
	for(i=0;aux[i]!=',';i++);
	char s[128];
	strncpy(s,aux,i);
	s[i]='\0';
	signal.scale=atof(s);
	aux+=i+1;
	signal.offset=atof(aux);
	return signal;
}
SG minMax(SG signal, char*token){
	char *aux=malloc(sizeof(char)*strlen(token)+1);
	strcpy(aux,token);
	int i;
	aux++;
	aux[strlen(aux)-1]='\0';
	for(i=0;aux[i]!='|';i++);
	char s[128];
	strncpy(s,aux,i);
	s[i]='\0';
	signal.min=atof(s);
	aux+=i+1;
	signal.max=atof(aux);
	return signal;
}
SG signalBits(SG signal,char* token){
	char *aux=malloc(sizeof(char)*strlen(token)+1);
	strcpy(aux,token);
	int j;
	for(j=0;aux[j]!='|';j++);
	char bitS[j+1];
	strncpy(bitS,aux,j);
	bitS[j]='\0';
	signal.bitStart=atoi(bitS);
	aux+=j+1;
	/*String after the | character*/
	for(j=0;aux[j]!='@';j++);
	char len[j+1];
	strncpy(len,aux,j);
	len[j]='\0';
	signal.length=atoi(len);
	aux+=j+1;
	/*String after the @*/
	signal.endian=aux[0]-'0';
	if(aux[1]=='+')
		signal.signFlag=1;
	else
		signal.signFlag=0;
	return signal;
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
				/*Ids in dbc file appear to be offset by 254*/
				idLine-=254;
				/*Decode ID*/
				res->messageID=idLine+254;
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
	char* unit=malloc(sizeof(char)*1024);
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
				res=signalBits(res,token);
				break;
			case 4:
				//(1,0)
				res=scaleBits(res,token);
				break;
			case 5:
				//[0|3]
				res=minMax(res,token);
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
		char *lineaux = malloc(sizeof(char)*100000);
		strcpy(lineaux,line);
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
				memcpy(boList,boList2,sizeof(BO_List));
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
				memcpy(boList->list[lastInserted].signals,&sgList,sizeof(SG_List));
			}
			boList->list[lastInserted].signals->list[nSignals]=signal;
			boList->list[lastInserted].signals->current++;
		}else if(strcmp(bo_,"CM_ ")==0){
			//CM_ BO_ 2566739710 "Class C, Pending";
			strtok(line, "\n");
			char* token=strtok(line," ");
			int j=0;
			int flag=-1;
			long messageID;
			while(token!=NULL && j<3){
				switch(j){
					case 0:
						break;
					case 1:
						if(strcmp(token,"BO_")==0){
							flag=0;
						}else if(strcmp(token,"SG_")==0){
							flag=1;
						}
						break;
					case 2:
						if(flag==0){
							messageID=atol(token);
							for(int i=0;i<boList->current && flag==0;i++){
								if(messageID==boList->list[i].messageID){
									int k=0,f=0;
									char aux[1024];
									for(int k=0,j=0;k<strlen(lineaux) && f>=0 ;k++){
										if(lineaux[k]!='\"' && f==0)
											continue;
										else if(lineaux[k]=='\"' && f==0){
											f=1;
										}else if(lineaux[k]!='\"' && f==1){
											aux[j]=lineaux[k];
											j++;
										}else if(lineaux[k]=='\"' && f==1){
											aux[j]='\0';
											f=-1;
										}
									}
									free(lineaux);
									strcpy(boList->list[i].description,aux);
									flag=-1;
								}else{
									continue;
								}
							}
						}
						if(flag==1){
							messageID=atol(token);
							token=strtok(NULL," ");
							for(int i=0;i<boList->current && flag==1;i++){
								if(messageID==boList->list[i].messageID){
									for(int j=0;j<boList->list[i].signals->current && flag==1;j++){	
										if(token!=NULL){				
											if(strcmp(token,boList->list[i].signals->list[j].name)==0){
												int k=0,f=0;
												char aux[1024];
												for(int k=0,j=0;k<strlen(lineaux) && f>=0 ;k++){
													if(lineaux[k]!='\"' && f==0)
														continue;
													else if(lineaux[k]=='\"' && f==0){
														f=1;
													}else if(lineaux[k]!='\"' && f==1){
														aux[j]=lineaux[k];
														j++;
													}else if(lineaux[k]=='\"' && f==1){
														aux[j]='\0';
														f=-1;
													}
												}
												free(lineaux);
												strcpy(boList->list[i].signals->list[j].description,aux);
												flag=-1;
											}else{
												continue;
											}
										}
									}
								}
							}
						}
						break;
					default:
						break;
				}
				token=strtok(NULL," ");
				j++;
			}
		}
    }
	for(int i=0;i<boList->current;i++)
		for(int j=0;j<boList->list[i].signals->current;j++)
			if(strlen(boList->list[i].signals->list[j].description)==0)
				strcpy(boList->list[i].signals->list[j].description,"N/A");
    fclose(fp);
	if(line)
    	free(line);
    return boList;
}
void parseCAN(BO_List* boList,int fd[])
{
	int s, i; 
	int nbytes;
	struct sockaddr_can addr;
	struct ifreq ifr;
	struct can_frame frame;

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Socket");
		exit(0);
	}

	strcpy(ifr.ifr_name, "vcan0" );
	ioctl(s, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Bind");
		exit(0);
	}
    while(1){
		decodedCAN* dc=(decodedCAN*)malloc(sizeof(decodedCAN));
        nbytes = read(s, &frame, sizeof(struct can_frame));

        if (nbytes < 0) {
            perror("Read");
            exit(0);
        }
		unsigned char data[frame.can_dlc];
		/*frame.data is inverted for some reason*/
		for (int j=0,i = frame.can_dlc-1; i >=0; i--,j++){
			data[j]=frame.data[i];
		}
		dc=decode(intToHex(frame.can_id),frame.can_dlc,data,boList,dc);
		write(fd[1],dc,sizeof(decodedCAN));
		free(dc);
    }

	if (close(s) < 0) {
		perror("Close");
		exit(0);
	}
}