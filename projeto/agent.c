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
    BO_List *boList = readDBC("SocketCAN/J1939/J1939.dbc");
    /*Error Group*/
    init_errorDescriptionTable();
    init_errorTable();
    /*********************************/
    /*Sensor Group*/
    init_mapTypeTable(boList);
    init_capabilitiesTable();
    init_requestControlDataTable();
    init_requestMonitoringDataTable();
    init_requestStatisticsDataTable();
    init_samplesTable();
    /*********************************/
    /*Actuator Group*/
    init_commandTable();
    /*********************************/
    init_snmp("veicular-daemon");
    /* If we're going to be a snmp master agent, initial the ports */
    if (!agentx_subagent)
        init_master_agent(); /* open the port to listen on 
        (defaults to udp:161) */

    /* In case we recevie a request to stop (kill -TERM or kill -INT) */
    keep_running = 1;
    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);
    signal(SIGCHLD, SIG_IGN);
    ssize_t r = 0;
    int fd[2];
    if (pipe(fd) < 0)
        exit(1);
    //nMessage will be used in the creation of checksum, incase 2 messages from same node arrive in close proximity, it's type is long due to it's higher maximum number
    long long nMessage;
    canDecoder = fork();
    /* you're main loop here... */
    if (canDecoder == 0)
    {
        parseCAN(boList, fd);
    }
    else
    {
        while (keep_running)
        {
            checkActuators();
            checkTables();
            /* if you use select(), see snmp_select_info() in snmp_api(3) */
            /*     --- OR ---  */
            agent_check_and_process(0); /* 0 == don't block */
            decodedCAN dc;
            int retval = fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK);
            r = read(fd[0], &dc, sizeof(decodedCAN));
            if (r <= 0)
            {
                dc.signals = -1;
            }
            else
            {
                if (dc.signals >= 0)
                {
                    char nMessageString[19];
                    sprintf(nMessageString, "%lld", nMessage);
                    nMessage++;
                    time_t t = time(NULL);
                    struct tm *tm = localtime(&t);
                    char s[100];
                    snprintf(s, 100, "%02d/%02d/%04d %02d:%02d:%02d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
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
    }
    /* at shutdown time */
    snmp_shutdown("veicular-daemon");
    SOCK_CLEANUP;
    return 1;
}