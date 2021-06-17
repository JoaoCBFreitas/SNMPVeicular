#include <agent.h>

static int keep_running;
//child process to decode CAN messages
pid_t canDecoder=-1;
RETSIGTYPE
stop_server(int a)
{
    keep_running = 0;
    kill(canDecoder,SIGKILL);
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

    init_errorSensorTable();
    /*********************************/
    init_mapTypeTable(boList);
    init_capabilitiesTable();
    init_requestControlDataTable();
    init_requestMonitoringDataTable();
    init_requestStatisticsDataTable();
    init_samplesTable();
    init_sampledValuesTable();
    init_errorTable();
    init_snmp("veicular-daemon");
    /* If we're going to be a snmp master agent, initial the ports */
    if (!agentx_subagent)
        init_master_agent(); /* open the port to listen on 
        (defaults to udp:161) */

    /* In case we recevie a request to stop (kill -TERM or kill -INT) */
    keep_running = 1;
    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);
    signal(SIGCHLD,SIG_IGN);
    ssize_t r=0;
    int fd[2];
    if(pipe(fd)<0)
        exit(1);

    canDecoder=fork();
    /* you're main loop here... */
    if(canDecoder==0){
        parseCAN(boList,fd);
    }else{
        while (keep_running)
        {
            checkTables();
            /* if you use select(), see snmp_select_info() in snmp_api(3) */
            /*     --- OR ---  */
            agent_check_and_process(0); /* 0 == don't block */
            decodedCAN dc;
            int retval=fcntl(fd[0],F_SETFL,fcntl(fd[0],F_GETFL)| O_NONBLOCK);
            r=read(fd[0],&dc,sizeof(decodedCAN));
            if(r<=0){
                dc.signals=-1;
            }else{
                for(int i=0;i<dc.signals;i++)
                    checkSamples(dc.signalname[i],dc.value[i],dc.signals);
            }
        }
    }    
    /* at shutdown time */
    snmp_shutdown("veicular-daemon");
    SOCK_CLEANUP;
    return 1;
}