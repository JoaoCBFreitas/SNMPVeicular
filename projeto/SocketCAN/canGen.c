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
	float timestamp;
	unsigned char* id;
	int dlc;
	unsigned char data[16];
}can;
typedef struct canlist{
	int capacity;
	int current;
	can* list;
}canlist;

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

    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1)
    {
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
					strcpy(s,token);
					s[2]='.';
					aux->timestamp=atof(s);
					break;
				case 1:
					strcpy(aux->id,token);
					break;
				case 2:
					aux->dlc=atoi(token);
					break;
				default:
					strcat(aux->data,token);
					break;
			}
			token=strtok(NULL," ");
			j++;
		}
        i++;
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
	canlist* list=readDump("rostselmash.trc");
	for(int i=0;i<list->current;){
		frame.can_id=strtoul(list->list[i].id,NULL,16);
		frame.can_dlc = list->list[i].dlc;
		long unsigned int aux=strtoul(list->list[i].data,NULL,16);
		printf("%lX\n",aux);
		memcpy(frame.data,&aux,sizeof(aux));
		if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
			perror("Write");
			return 1;
		}
		i++;
		usleep(1250);
		if(i==list->current){
			i=0;
			sleep(10);
		}
	}


	if (close(s) < 0) {
		perror("Close");
		return 1;
	}

	return 0;
}