#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <linux/can.h>
#include <linux/can/raw.h>
typedef struct can{
	double timestamp;
	unsigned char* id;
	int dlc;
	unsigned char data[128];
}can;
typedef struct canlist{
	int capacity;
	int current;
	can* list;
}canlist;

void printEscolha(){
	printf("****************\n");
	printf("*  Choose log  *\n");
	printf("*1-Log 00002081*\n");
	printf("*2-Log 00002082*\n");
	printf("*3-Log 00002083*\n");
	printf("*4-Log 00002084*\n");
	printf("****************\n");

}
canlist* readDump(char* file){
	canlist* canList = (canlist *)malloc(sizeof(canlist));
    canList->capacity = 1;
    canList->current = 0;
    canList->list = malloc(sizeof(can));
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen(file, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

	int lineF=0;
    while ((read = getline(&line, &len, fp)) != -1)
    {
		lineF++;
		if(lineF<4) continue;
        can *aux = (can *)malloc(sizeof(can));
		aux->id=malloc(sizeof(char)*30);
		strcpy(aux->data,"");
        strtok(line, "\n");
		char* token=strtok(line," ");
		int j=0;
		while(token!=NULL){
			char s[6];
			switch(j){
				case 0:
					break;
				case 1:
					strcpy(s,token);
					aux->timestamp=atof(s);
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					strcpy(aux->id,token);
					break;
				case 5:
					break;
				case 6:
					aux->dlc=atoi(token);
					break;
				default:
					strcat(aux->data,token);
					break;
			}
			token=strtok(NULL," ");
			j++;
		}
        if (canList->capacity == canList->current)
        {
            canlist *canList2 = (canlist *)malloc(sizeof(canlist));
            canList2->capacity = canList->capacity * 2;
            canList2->current = canList->current;
            canList2->list = malloc(sizeof(can) * canList2->capacity);
            for (int j = 0; j < canList->current; j++)
            {
                canList2->list[j] = canList->list[j];
            }
            canList->capacity = canList2->capacity;
            canList->current = canList2->current;
            canList->list = canList2->list;
            free(canList2);
        }
        canList->list[canList->current] = *aux;
        canList->current++;
		free(aux);
    }
    fclose(fp);
    if (line)
        free(line);
    return canList;
}
int main(int argc, char **argv)
{
	int s; 
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
	int choice =-1;
	canlist* list;
	while(choice==-1){
		printEscolha();
		scanf("%d",&choice);
		switch(choice){
			case 1:
				list=readDump("SocketCAN/J1939/00002081_CAN.trc");
				break;
			case 2:
				list=readDump("SocketCAN/J1939/00002082_CAN.trc");
				break;
			case 3:
				list=readDump("SocketCAN/J1939/00002083_CAN.trc");
				break;
			case 4:
				list=readDump("SocketCAN/J1939/00002084_CAN.trc");
				break;
			default:
				choice=-1;
				printf("Wrong input\n");
				break;
		}
	}
	for(int i=0;i<list->current;){
		frame.can_id=strtoul(list->list[i].id,NULL,16);
		frame.can_dlc = list->list[i].dlc;
		long unsigned int aux=strtoul(list->list[i].data,NULL,16);
		printf("%s %lX\n",list->list[i].id,aux);
		memcpy(frame.data,&aux,sizeof(aux));
		if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
			perror("Write");
			return 1;
		}
		
		if(i<list->current-1){
			int sleeptime=(int)((list->list[i+1].timestamp-list->list[i].timestamp)*1000000);
			usleep(sleeptime);
		}
		
		//usleep(100000);
		i++;
	}


	if (close(s) < 0) {
		perror("Close");
		return 1;
	}
	free(list);
	printf("\n\n\n");
	printf("Log is over\n");
	return 0;
}