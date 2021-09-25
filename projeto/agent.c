#include <agent.h>

static int keep_running;
//child process to decode CAN messages
pid_t canDecoder = -1;
RETSIGTYPE
stop_server(int a)
{
    keep_running = 0;
    kill(canDecoder, SIGKILL);
}
/*This function will read the contents of cache.bin and add them to the MIB,
The contents of this cache are as follows:
    syscache (which will contain the current number of entries for each of the 4 tables)
    0..n entries of requestMonitoringDataTable
    0..n entries of requestControlDataTable
    0..n entries of requestStatisticsDataTable
    0..n entries of samplesTable
    */
void readCache()
{
    int cachefd = open("cache.bin", O_RDONLY, 0644);
    if (cachefd < 0)
    {
        printf("No cache found\n");
        close(cachefd);
    }
    else
    {
        systemCache sysCache;
        lseek(cachefd, 0, SEEK_SET);
        ssize_t readByte = 0;
        int byte = 1;
        byte = read(cachefd, &sysCache, sizeof(systemCache));
        while (byte > 0)
        {
            /*Reading cached requestMonitoringDataEntries and adding them to the agent*/
            if (sysCache.mc.current > 0)
            {
                requestMonitoringDataTable_context *req = (requestMonitoringDataTable_context *)malloc(sizeof(requestMonitoringDataTable_context));
                requestMonitoringStruct *reqStruct = (requestMonitoringStruct *)malloc(sizeof(requestMonitoringStruct));
                for (int i = 0; i < sysCache.mc.current; i++)
                {
                    byte = read(cachefd, &req->requestID, sizeof(unsigned long));
                    byte = read(cachefd, &req->requestControlID, sizeof(unsigned long));
                    byte = read(cachefd, &req->requestMapID, sizeof(unsigned long));
                    byte = read(cachefd, &req->requestStatisticsID, sizeof(unsigned long));
                    byte = read(cachefd, &req->savingMode, sizeof(long));
                    byte = read(cachefd, &req->samplingFrequency, sizeof(unsigned long));
                    byte = read(cachefd, &req->maxDelay, sizeof(long));
                    byte = read(cachefd, &req->startTime_len, sizeof(long));
                    byte = read(cachefd, &req->startTime, sizeof(unsigned char) * req->startTime_len);
                    byte = read(cachefd, &req->endTime_len, sizeof(long));
                    byte = read(cachefd, &req->endTime, sizeof(unsigned char) * req->endTime_len);
                    byte = read(cachefd, &req->waitTime_len, sizeof(long));
                    byte = read(cachefd, &req->waitTime, sizeof(unsigned char) * req->waitTime_len);
                    byte = read(cachefd, &req->durationTime_len, sizeof(long));
                    byte = read(cachefd, &req->durationTime, sizeof(unsigned char) * req->durationTime_len);
                    byte = read(cachefd, &req->expireTime_len, sizeof(long));
                    byte = read(cachefd, &req->expireTime, sizeof(unsigned char) * req->expireTime_len);
                    byte = read(cachefd, &req->maxNOfSamples, sizeof(unsigned long));
                    byte = read(cachefd, &req->lastSampleID, sizeof(unsigned long));
                    byte = read(cachefd, &req->loopMode, sizeof(long));
                    byte = read(cachefd, &req->nOfSamples, sizeof(unsigned long));
                    byte = read(cachefd, &req->status, sizeof(long));
                    byte = read(cachefd, &req->requestUser_len, sizeof(long));
                    byte = read(cachefd, &req->requestUser, sizeof(unsigned char) * req->requestUser_len);
                    reqStruct = tableToStruct(req, reqStruct);
                    insertMonitoringRow(reqStruct);
                    free(reqStruct->startTime);
                    free(reqStruct->waitTime);
                    free(reqStruct->durationTime);
                    free(reqStruct->expireTime);
                    free(reqStruct->endTime);
                    free(reqStruct->requestUser);
                }
                free(reqStruct);
                free(req);
            }
            /*Reading cached requestControlDataEntries and adding them to the agent*/
            if (sysCache.cc.current > 0)
            {
                requestControlDataTable_context *req = (requestControlDataTable_context *)malloc(sizeof(requestControlDataTable_context));
                requestStruct *reqStruct = (requestStruct *)malloc(sizeof(requestStruct));
                for (int i = 0; i < sysCache.cc.current; i++)
                {
                    byte = read(cachefd, &req->requestControlID, sizeof(unsigned long));
                    byte = read(cachefd, &req->requestControlMapID, sizeof(unsigned long));
                    byte = read(cachefd, &req->settingMode, sizeof(long));
                    byte = read(cachefd, &req->commitTime_len, sizeof(long));
                    byte = read(cachefd, &req->commitTime, sizeof(unsigned char) * req->commitTime_len);
                    byte = read(cachefd, &req->endControlTime_len, sizeof(long));
                    byte = read(cachefd, &req->endControlTime, sizeof(unsigned char) * req->endControlTime_len);
                    byte = read(cachefd, &req->durationControlTime_len, sizeof(long));
                    byte = read(cachefd, &req->durationControlTime, sizeof(unsigned char) * req->durationControlTime_len);
                    byte = read(cachefd, &req->expireControlTime_len, sizeof(long));
                    byte = read(cachefd, &req->expireControlTime, sizeof(unsigned char) * req->expireControlTime_len);
                    byte = read(cachefd, &req->valuesTableID, sizeof(unsigned long));
                    byte = read(cachefd, &req->statusControl, sizeof(long));
                    reqStruct = reqControlConvert(req, reqStruct);
                    insertControlRow(reqStruct);
                    free(reqStruct->commitTime);
                    free(reqStruct->endTime);
                    free(reqStruct->duration);
                    free(reqStruct->expireTime);
                }
                free(reqStruct);
                free(req);
            }
            /*Reading cached requestStatisticsDataEntries and adding them to the agent*/
            if (sysCache.sc.current > 0)
            {
                requestStatisticsDataTable_context *req = (requestStatisticsDataTable_context *)malloc(sizeof(requestStatisticsDataTable_context));
                statisticsStruct *reqStruct = (statisticsStruct *)malloc(sizeof(statisticsStruct));
                for (int i = 0; i < sysCache.sc.current; i++)
                {
                    byte = read(cachefd, &req->statisticsID, sizeof(unsigned long));
                    byte = read(cachefd, &req->durationTimeStatistics_len, sizeof(long));
                    byte = read(cachefd, &req->durationTimeStatistics, sizeof(unsigned char) * req->durationTimeStatistics_len);
                    byte = read(cachefd, &req->nOfSamplesStatistics, sizeof(unsigned long));
                    byte = read(cachefd, &req->minValue, sizeof(long));
                    byte = read(cachefd, &req->maxValue, sizeof(long));
                    byte = read(cachefd, &req->avgValue, sizeof(long));
                    reqStruct = convertStatStruct(req, reqStruct);
                    insertStatisticsRow(reqStruct);
                    free(reqStruct->duration);
                }
                free(reqStruct);
                free(req);
            }
            /*Reading cached samplesEntries and adding them to the agent*/
            if (sysCache.rc.current > 0)
            {
                samplesTable_context *req = (samplesTable_context *)malloc(sizeof(samplesTable_context));
                samplesStruct *reqStruct = (samplesStruct *)malloc(sizeof(samplesStruct));
                for (int i = 0; i < sysCache.rc.current; i++)
                {
                    byte = read(cachefd, &req->sampleID, sizeof(unsigned long));
                    byte = read(cachefd, &req->requestSampleID, sizeof(unsigned long));
                    byte = read(cachefd, &req->timeStamp_len, sizeof(long));
                    byte = read(cachefd, &req->timeStamp, sizeof(unsigned char) * req->timeStamp_len);
                    byte = read(cachefd, &req->sampleFrequency, sizeof(unsigned long));
                    byte = read(cachefd, &req->previousSampleID, sizeof(unsigned long));
                    byte = read(cachefd, &req->sampleType, sizeof(unsigned long));
                    byte = read(cachefd, &req->sampleRecordedValue, sizeof(unsigned long));
                    byte = read(cachefd, &req->mapTypeSamplesID, sizeof(unsigned long));
                    byte = read(cachefd, &req->sampleChecksum_len, sizeof(long));
                    byte = read(cachefd, &req->sampleChecksum, sizeof(unsigned char) * req->sampleChecksum_len);
                    reqStruct = sampleTableToStruct(req, reqStruct);
                    insertSamplesRow(reqStruct);
                    free(reqStruct->sampleCheckSum);
                    free(reqStruct->timestamp);
                }
                free(reqStruct);
                free(req);
            }
            break;
        }
        close(cachefd);
    }
}

/*This function will write the contents of requestMonitoringDataTable_context to a file descriptor*/
int serializeRequestMonitoring(requestMonitoringDataTable_context *req, int cachefd)
{
    int res = 0;
    res += write(cachefd, &req->requestID, sizeof(unsigned long));
    res += write(cachefd, &req->requestControlID, sizeof(unsigned long));
    res += write(cachefd, &req->requestMapID, sizeof(unsigned long));
    res += write(cachefd, &req->requestStatisticsID, sizeof(unsigned long));
    res += write(cachefd, &req->savingMode, sizeof(long));
    res += write(cachefd, &req->samplingFrequency, sizeof(unsigned long));
    res += write(cachefd, &req->maxDelay, sizeof(long));
    res += write(cachefd, &req->startTime_len, sizeof(long));
    res += write(cachefd, &req->startTime, sizeof(unsigned char) * req->startTime_len);
    res += write(cachefd, &req->endTime_len, sizeof(long));
    res += write(cachefd, &req->endTime, sizeof(unsigned char) * req->endTime_len);
    res += write(cachefd, &req->waitTime_len, sizeof(long));
    res += write(cachefd, &req->waitTime, sizeof(unsigned char) * req->waitTime_len);
    res += write(cachefd, &req->durationTime_len, sizeof(long));
    res += write(cachefd, &req->durationTime, sizeof(unsigned char) * req->durationTime_len);
    res += write(cachefd, &req->expireTime_len, sizeof(long));
    res += write(cachefd, &req->expireTime, sizeof(unsigned char) * req->expireTime_len);
    res += write(cachefd, &req->maxNOfSamples, sizeof(unsigned long));
    res += write(cachefd, &req->lastSampleID, sizeof(unsigned long));
    res += write(cachefd, &req->loopMode, sizeof(long));
    res += write(cachefd, &req->nOfSamples, sizeof(unsigned long));
    res += write(cachefd, &req->status, sizeof(long));
    res += write(cachefd, &req->requestUser_len, sizeof(long));
    res += write(cachefd, &req->requestUser, sizeof(unsigned char) * req->requestUser_len);
    return res;
}

/*This function will write the contents of requestControlDataTable_context to a file descriptor*/
int serializeRequestControl(requestControlDataTable_context *req, int cachefd)
{
    int res = 0;
    res += write(cachefd, &req->requestControlID, sizeof(unsigned long));
    res += write(cachefd, &req->requestControlMapID, sizeof(unsigned long));
    res += write(cachefd, &req->settingMode, sizeof(long));
    res += write(cachefd, &req->commitTime_len, sizeof(long));
    res += write(cachefd, &req->commitTime, sizeof(unsigned char) * req->commitTime_len);
    res += write(cachefd, &req->endControlTime_len, sizeof(long));
    res += write(cachefd, &req->endControlTime, sizeof(unsigned char) * req->endControlTime_len);
    res += write(cachefd, &req->durationControlTime_len, sizeof(long));
    res += write(cachefd, &req->durationControlTime, sizeof(unsigned char) * req->durationControlTime_len);
    res += write(cachefd, &req->expireControlTime_len, sizeof(long));
    res += write(cachefd, &req->expireControlTime, sizeof(unsigned char) * req->expireControlTime_len);
    res += write(cachefd, &req->valuesTableID, sizeof(unsigned long));
    res += write(cachefd, &req->statusControl, sizeof(long));
    return res;
}

/*This function will write the contents of requestStatisticsDataTable_context to a file descriptor*/
int serializeRequestStatistics(requestStatisticsDataTable_context *req, int cachefd)
{
    int res = 0;
    res += write(cachefd, &req->statisticsID, sizeof(unsigned long));
    res += write(cachefd, &req->durationTimeStatistics_len, sizeof(long));
    res += write(cachefd, &req->durationTimeStatistics, sizeof(unsigned char) * req->durationTimeStatistics_len);
    res += write(cachefd, &req->nOfSamplesStatistics, sizeof(unsigned long));
    res += write(cachefd, &req->minValue, sizeof(long));
    res += write(cachefd, &req->maxValue, sizeof(long));
    res += write(cachefd, &req->avgValue, sizeof(long));
    return res;
}

/*This function will write the contents of samplesTable_context to a file descriptor*/
int serializeSamples(samplesTable_context *req, int cachefd)
{
    int res = 0;
    res += write(cachefd, &req->sampleID, sizeof(unsigned long));
    res += write(cachefd, &req->requestSampleID, sizeof(unsigned long));
    res += write(cachefd, &req->timeStamp_len, sizeof(long));
    res += write(cachefd, &req->timeStamp, sizeof(unsigned char) * req->timeStamp_len);
    res += write(cachefd, &req->sampleFrequency, sizeof(unsigned long));
    res += write(cachefd, &req->previousSampleID, sizeof(unsigned long));
    res += write(cachefd, &req->sampleType, sizeof(unsigned long));
    res += write(cachefd, &req->sampleRecordedValue, sizeof(unsigned long));
    res += write(cachefd, &req->mapTypeSamplesID, sizeof(unsigned long));
    res += write(cachefd, &req->sampleChecksum_len, sizeof(long));
    res += write(cachefd, &req->sampleChecksum, sizeof(unsigned char) * req->sampleChecksum_len);
    return res;
}

/*This function will add entries from requestMonitoringDataTable, requestControlDataTable, requestStatisticsDataTable and samplesTable to the cache file*/
void createCache(systemCache sc)
{
    int cachefd = open("cache.bin", O_TRUNC | O_RDWR | O_CREAT, 0644);
    if (cachefd < 0)
    {
        perror("Error opening file cache.bin\n");
        close(cachefd);
    }
    else
    {
        lseek(cachefd, 0, SEEK_END);
        int written = write(cachefd, &sc, sizeof(systemCache));
        for (int i = 0; i < sc.mc.current; i++)
        {
            written += serializeRequestMonitoring(sc.mc.items[i], cachefd);
            free(sc.mc.items[i]);
        }
        for (int i = 0; i < sc.cc.current; i++)
        {
            written += serializeRequestControl(sc.cc.items[i], cachefd);
            free(sc.cc.items[i]);
        }
        for (int i = 0; i < sc.sc.current; i++)
        {
            written += serializeRequestStatistics(sc.sc.items[i], cachefd);
            free(sc.sc.items[i]);
        }
        for (int i = 0; i < sc.rc.current; i++)
        {
            written += serializeSamples(sc.rc.items[i], cachefd);
            free(sc.rc.items[i]);
        }
        close(cachefd);
    }
}

int main(int argc, char **argv)
{
    int agentx_subagent = 1; /* change this if you want to be a SNMP master agent */
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
    BO_List *boList = readDBC("SocketCAN/J1939/J1939.dbc");
    /*Initializing Error Group*/
    init_errorDescriptionTable();
    init_errorTable();
    /*********************************/
    /*Initializing Sensor Group*/
    init_mapTypeTable(boList);
    init_capabilitiesTable();
    init_requestControlDataTable();
    init_requestMonitoringDataTable();
    init_requestStatisticsDataTable();
    init_samplesTable();
    /*********************************/
    /*Initialize Actuator Group*/
    init_commandTemplateTable();
    init_commandTable();
    /*********************************/
    init_snmp("veicular-daemon");
    /* If we're going to be a snmp master agent, initial the ports */
    if (!agentx_subagent)
        init_master_agent(); /* open the port to listen on 
        (defaults to udp:161) */
    /*Read Contents from cache.bin and add them to the agent*/
    readCache();
    keep_running = 1;
    ssize_t r = 0;
    int fd[2];
    if (pipe(fd) < 0)
        exit(1);
    /* nMessage will be used in the creation of checksum, incase 2 messages from same node arrive in close proximity, 
    it's type is long due to it's higher maximum number*/
    long long nMessage;
    canDecoder = fork();
    /* you're main loop here... */
    if (canDecoder == 0)
    {
        /*Child process will only decode CAN messages*/
        parseCAN(boList, fd);
        exit(0);
    }
    else
    {
        /*sigaction is apparently more portable than signal*/
        struct sigaction act;
        act.sa_handler = stop_server;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
        sigaction(SIGQUIT, &act, NULL);
        while (keep_running)
        {
            checkTables();
            /* if you use select(), see snmp_select_info() in snmp_api(3) */
            /*     --- OR ---  */
            if (agent_check_and_process(0) > 0) /* 0 == don't block */
                /*agent_check_and_process will return 1 when it receives a packet, 
                there's no need to check actuator table if no packet is received*/
                checkActuators();
            decodedCAN dc;
            /*Ensure that read() doesn't block*/
            int retval = fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK);
            r = read(fd[0], &dc, sizeof(decodedCAN));
            if (r > 0 && dc.signals >= 0)
            {
                /*if r<=0 that means that no data was read, while if dc.signals>=0 
                means that the decoder was able to decode the message*/
                char nMessageString[19];
                sprintf(nMessageString, "%lld", nMessage);
                nMessage++;
                time_t t = time(NULL);
                struct tm *tm = localtime(&t);
                char s[100];
                snprintf(s, 100, "%02d/%02d/%04d %02d:%02d:%02d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
                /*Aux will contain the string with which the checksum is created-> timestamp.NMessage.EcuName*/
                char *aux = malloc(sizeof(char) * (strlen(s) + strlen(dc.name) + strlen(nMessageString) + 1));
                strcpy(aux, s);
                strcat(aux, nMessageString);
                strcat(aux, dc.name);
                char *check = createChecksum(aux);
                free(aux);
                for (int i = 0; i < dc.signals; i++)
                {
                    char *signalname = malloc(sizeof(char) * strlen(dc.signalname[i]) + strlen(dc.name) + 1);
                    strcpy(signalname, dc.name);
                    strcat(signalname, dc.signalname[i]);
                    checkSamples(signalname, dc.value[i], dc.signals, s, check);
                    free(signalname);
                }
            }
        }
    }
    /*Shutdown procedure begun, in here we will first clear all "volatile" entries in requestMonitoringDataTable and then 
    store the remaining entries in a cache file for later use.
    To do this we must first find all entries with savingMode set to 1 and then change loopMode to 2 and status to delete*/
    clearVolatileEntries();
    /*To clear those entries, we can run checkTables() until no volatile entries are found*/
    while (checkVolatileRequests() != 1)
        checkTables();
    /*We can now traverse requestMonitoringDataTable and store the remaining entries, and those related to them, in a cache file*/
    systemCache sc = cacheEntries();
    /*Store sc in a file*/
    createCache(sc);
    /* at shutdown time */
    snmp_shutdown("veicular-daemon");
    SOCK_CLEANUP;
    return 1;
}
