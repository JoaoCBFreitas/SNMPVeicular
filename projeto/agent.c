#include <agent.h>

static int keep_running;

RETSIGTYPE
stop_server(int a)
{
    keep_running = 0;
}
sampleUnitsList *readSampleUnits(sampleUnitsList *samples,BO_List *boList)
{
    samples = (sampleUnitsList *)malloc(sizeof(sampleUnitsList));
    samples->capacity = 1;
    samples->current = 0;
    samples->list = malloc(sizeof(sampleUnitsStruct));    
    int f = 0,inserted=0;
    for(int i=0;i<boList->current;i++){
        for(int j=0;j<boList->list[i].signals->current;j++){
            for(int k=0;k<samples->current;k++){
                if(strcmp(samples->list[k].unit,boList->list[i].signals->list[j].unit)==0 || strcmp("",boList->list[i].signals->list[j].unit)==0)
                    f=1;
            }
            if(f==0){
                sampleUnitsStruct *aux = (sampleUnitsStruct *)malloc(sizeof(sampleUnitsStruct));
                aux->id=inserted;
                inserted++;   
                aux->unit = malloc(sizeof(char) * 1024);
                strcpy(aux->unit,boList->list[i].signals->list[j].unit);
                if (samples->capacity == samples->current)
                {
                    sampleUnitsList *samples2 = (sampleUnitsList *)malloc(sizeof(sampleUnitsList));
                    samples2->capacity = samples->capacity * 2;
                    samples2->current = samples->current;
                    samples2->list = malloc(sizeof(sampleUnitsStruct) * samples2->capacity);
                    for (int j = 0; j < samples->current; j++)
                    {
                        samples2->list[j] = samples->list[j];
                    }
                    samples->capacity = samples2->capacity;
                    samples->current = samples2->current;
                    samples->list = samples2->list;
                    free(samples2);
                }
                samples->list[samples->current] = *aux;
                samples->current++;
            }
            f=0;
        }
    }
    return samples;
}
errorDescrList *readErrorDescr(errorDescrList *samples)
{
    samples = (errorDescrList *)malloc(sizeof(errorDescrList));
    samples->capacity = 1;
    samples->current = 0;
    samples->errorList = malloc(sizeof(errorDescrList));
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("./Files/errorDescription.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1)
    {
        errorDescrStruct *aux = (errorDescrStruct *)malloc(sizeof(errorDescrStruct));
        aux->errorDescrID = i;
        aux->errorDescr = malloc(sizeof(char) * 65535);
        strtok(line, "\n");
        strcpy(aux->errorDescr, line);
        if (samples->capacity == samples->current)
        {
            errorDescrList *samples2 = (errorDescrList *)malloc(sizeof(errorDescrList));
            samples2->capacity = samples->capacity * 2;
            samples2->current = samples->current;
            samples2->errorList = malloc(sizeof(errorDescrStruct) * samples2->capacity);
            for (int j = 0; j < samples->current; j++)
            {
                samples2->errorList[j] = samples->errorList[j];
            }
            samples->capacity = samples2->capacity;
            samples->current = samples2->current;
            samples->errorList = samples2->errorList;
            free(samples2);
        }
        i++;
        samples->errorList[samples->current] = *aux;
        samples->current++;
    }
    fclose(fp);
    if (line)
        free(line);
    return samples;
}
genericTypeList *readGenericTypes(genericTypeList *samples)
{
    samples = (genericTypeList *)malloc(sizeof(genericTypeList));
    samples->capacity = 1;
    samples->current = 0;
    samples->genericList = malloc(sizeof(genericTypeList));
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("./Files/genericTypes.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1)
    {
        genericTypeStruct *aux = (genericTypeStruct *)malloc(sizeof(genericTypeStruct));
        aux->genericTypeID = i;
        aux->typeDescription = malloc(sizeof(char) * 65535);
        strtok(line, "\n");
        strcpy(aux->typeDescription, line);
        if (samples->capacity == samples->current)
        {
            genericTypeList *samples2 = (genericTypeList *)malloc(sizeof(genericTypeList));
            samples2->capacity = samples->capacity * 2;
            samples2->current = samples->current;
            samples2->genericList = malloc(sizeof(genericTypeStruct) * samples2->capacity);
            for (int j = 0; j < samples->current; j++)
            {
                samples2->genericList[j] = samples->genericList[j];
            }
            samples->capacity = samples2->capacity;
            samples->current = samples2->current;
            samples->genericList = samples2->genericList;
            free(samples2);
        }
        i++;
        samples->genericList[samples->current] = *aux;
        samples->current++;
    }
    fclose(fp);
    if (line)
        free(line);
    return samples;
}
int main(int argc, char **argv)
{
    int agentx_subagent = 1; /* change this if you want to 
        be a SNMP master agent */
    int background = 0;      /* change this if you want to run in the background */
    int syslog = 0;          /* change this if you want to use syslog */

    /* print log errors to syslog or stderr */
    if (syslog)
        snmp_enable_calllog();
    else
        snmp_enable_stderrlog();
    //snmp_set_do_debugging(1);
    /* we're an agentx subagent? */
    if (agentx_subagent)
    {
        /* make us a agentx client. */
        netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID,
                               NETSNMP_DS_AGENT_ROLE, 1);
    }
    /* run in background, if requested */
    if (background && netsnmp_daemonize(1, !syslog))
        exit(1);
    /* initialize tcpip, if necessary */
    SOCK_STARTUP;
    DEBUGMSG(("Before agent library init", "\n"));
    /* initialize the agent library */
    init_agent("veicular-daemon");

    /* initialize mib code here */

    /*Initializing structs that contain dbc decoding rules*/
    BO_List* boList=readDBC("SocketCAN/J1939/J1939.dbc");
    /* These tables are populated by reading files */
    sampleUnitsList *sampleunitsList = readSampleUnits(sampleunitsList,boList);
    genericTypeList *typesList = readGenericTypes(typesList);
    errorDescrList *errorList = readErrorDescr(errorList);

    init_sampleUnitsTable(sampleunitsList);
    init_genericTypesTable(typesList);
    init_errorDescriptionTable(errorList);
    init_errorSensorTable();
    /*********************************/
    init_mapTypeTable();
    init_capabilitiesTable();
    init_requestControlDataTable();
    init_requestMonitoringDataTable();
    init_requestStatisticsDataTable();
    init_samplesTable();
    init_sampledValuesTable();
    init_errorTable();
    init_snmp("veicular-daemon");
    /*
    for(int i=0;i<boList->current;i++){
        printf("MessageDescription: %s\n",boList->list[i].description);
        for(int j=0;j<boList->list[i].signals->current;j++){
            printf("    SignalDescription: %s\n",boList->list[i].signals->list[j].description);
        }
    }
    */
    /* If we're going to be a snmp master agent, initial the ports */
    if (!agentx_subagent)
        init_master_agent(); /* open the port to listen on 
        (defaults to udp:161) */

    /* In case we recevie a request to stop (kill -TERM or kill -INT) */
    keep_running = 1;
    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);

    /* you're main loop here... */
    while (keep_running)
    {
        /* if you use select(), see snmp_select_info() in snmp_api(3) */
        /*     --- OR ---  */
        agent_check_and_process(1); /* 0 == don't block */
        checkTables();
    }

    /* at shutdown time */
    snmp_shutdown("veicular-daemon");
    SOCK_CLEANUP;
    return 1;
}