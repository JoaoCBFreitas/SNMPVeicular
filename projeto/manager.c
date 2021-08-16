#include <manager.h>

int set(netsnmp_session session, netsnmp_session *ss, oid *root, size_t rootlen, char **oidString, char *types, char **values, int current_name)
{
    int count;
    int failures = 0;
    int exitval = 0;
    int status;
    int quiet = 0;
    netsnmp_variable_list *vars;
    netsnmp_pdu *pdu, *response = NULL;
    pdu = snmp_pdu_create(SNMP_MSG_SET);
    /*
     * create PDU for SET request and add object names and values to request 
     */
    for (count = 0; count < current_name; count++)
    {
        get_node(oidString[count], root, &rootlen);
        if (snmp_add_var(pdu, root, rootlen, types[count], values[count]))
        {
            snmp_perror(oidString[count]);
            failures++;
        }
    }
    if (failures)
    {
        snmp_close(ss);
        SOCK_CLEANUP;
        exit(1);
    }
    /*
     * do the request 
     */
    status = snmp_synch_response(ss, pdu, &response);
    if (status == STAT_SUCCESS)
    {
        if (response->errstat == SNMP_ERR_NOERROR)
        {
            if (!quiet)
            {
                for (vars = response->variables; vars;
                     vars = vars->next_variable)
                    print_variable(vars->name, vars->name_length, vars);
            }
        }
        else
        {
            fprintf(stderr, "Error in packet.\nReason: %s\n",
                    snmp_errstring(response->errstat));
            if (response->errindex != 0)
            {
                fprintf(stderr, "Failed object: ");
                for (count = 1, vars = response->variables;
                     vars && (count != response->errindex);
                     vars = vars->next_variable, count++)
                    ;
                if (vars)
                    fprint_objid(stderr, vars->name, vars->name_length);
                fprintf(stderr, "\n");
            }
            exitval = 2;
        }
    }
    else if (status == STAT_TIMEOUT)
    {
        fprintf(stderr, "Timeout: No Response from %s\n",
                session.peername);
        exitval = 1;
    }
    else
    { /* status == STAT_ERROR */
        snmp_sess_perror("snmpset", ss);
        exitval = 1;
    }

    if (response)
        snmp_free_pdu(response);
    return exitval;
}

int bulkget(netsnmp_session session, netsnmp_session *ss, oid *root, size_t rootlen)
{
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    int status, running = 1, numprinted = 0, count, check, exitval = 0;
    netsnmp_variable_list *vars;
    oid name[MAX_OID_LEN];
    size_t name_length;
    memmove(name, root, rootlen * sizeof(oid));
    name_length = rootlen;
    while (running)
    {
        /*
         * create PDU for GETBULK request and add object name to request 
         */
        pdu = snmp_pdu_create(SNMP_MSG_GETBULK);
        pdu->non_repeaters = 1;
        pdu->max_repetitions = 20; /* fill the packet */
        snmp_add_null_var(pdu, name, name_length);

        /*
         * do the request 
         */
        status = snmp_synch_response(ss, pdu, &response);
        if (status == STAT_SUCCESS)
        {
            if (response->errstat == SNMP_ERR_NOERROR)
            {
                /*
                 * check resulting variables 
                 */
                for (vars = response->variables; vars;
                     vars = vars->next_variable)
                {
                    if ((vars->name_length < rootlen) || (memcmp(root, vars->name, rootlen * sizeof(oid)) != 0))
                    {
                        /*
                         * not part of this subtree 
                         */
                        running = 0;
                        continue;
                    }
                    numprinted++;
                    print_variable(vars->name, vars->name_length, vars);
                    if ((vars->type != SNMP_ENDOFMIBVIEW) &&
                        (vars->type != SNMP_NOSUCHOBJECT) &&
                        (vars->type != SNMP_NOSUCHINSTANCE))
                    {
                        /*
                         * not an exception value 
                         */
                        if (check && snmp_oid_compare(name, name_length,
                                                      vars->name,
                                                      vars->name_length) >= 0)
                        {
                            fprintf(stderr, "Error: OID not increasing: ");
                            fprint_objid(stderr, name, name_length);
                            fprintf(stderr, " >= ");
                            fprint_objid(stderr, vars->name,
                                         vars->name_length);
                            fprintf(stderr, "\n");
                            running = 0;
                            exitval = 1;
                        }
                        /*
                         * Check if last variable, and if so, save for next request.  
                         */
                        if (vars->next_variable == NULL)
                        {
                            memmove(name, vars->name,
                                    vars->name_length * sizeof(oid));
                            name_length = vars->name_length;
                        }
                    }
                    else
                    {
                        /*
                         * an exception value, so stop 
                         */
                        running = 0;
                    }
                }
            }
            else
            {
                /*
                 * error in response, print it 
                 */
                running = 0;
                if (response->errstat == SNMP_ERR_NOSUCHNAME)
                {
                    printf("End of MIB\n");
                }
                else
                {
                    fprintf(stderr, "Error in packet.\nReason: %s\n",
                            snmp_errstring(response->errstat));
                    if (response->errindex != 0)
                    {
                        fprintf(stderr, "Failed object: ");
                        for (count = 1, vars = response->variables;
                             vars && count != response->errindex;
                             vars = vars->next_variable, count++)
                            /*EMPTY*/;
                        if (vars)
                            fprint_objid(stderr, vars->name,
                                         vars->name_length);
                        fprintf(stderr, "\n");
                    }
                    exitval = 2;
                }
            }
        }
        else if (status == STAT_TIMEOUT)
        {
            fprintf(stderr, "Timeout: No Response from %s\n",
                    session.peername);
            running = 0;
            exitval = 1;
        }
        else
        { /* status == STAT_ERROR */
            snmp_sess_perror("snmpbulkwalk", ss);
            running = 0;
            exitval = 1;
        }
    }
    if (response)
        snmp_free_pdu(response);
    return exitval;
}
void printMenu()
{
    printf("********************************\n");
    printf("*RequestMonitoringDataTable -1 *\n");
    printf("*RequestControlDataTable    -2 *\n");
    printf("*RequestStatisticsDataTable -3 *\n");
    printf("*CapabilitiesTable          -4 *\n");
    printf("*SamplesTable               -5 *\n");
    printf("*MapTypeTable               -6 *\n");
    printf("*GenericTypesTable          -7 *\n");
    printf("*SampleUnitsTable           -8 *\n");
    printf("*ErrorTable                 -9*\n");
    printf("*ErrorDescriptionTable      -10*\n");
    printf("*CommandTemplateTable       -11*\n");
    printf("*CommandTable               -12*\n");
    printf("*Set Teste                  -13*\n");
    printf("*Sair                       -0 *\n");
    printf("********************************\n");
}
int main(int argc, char **argv)
{
    netsnmp_session session, *ss;

    /*
     * Initialize the SNMP library
     */
    init_snmp("snmpadmin");

    /*
     * Initialize a "session" that defines who we're going to talk to
     */
    snmp_sess_init(&session); /* set up defaults */
    session.peername = strdup("localhost");

    /* set up the authentication parameters for talking to the server */

    /* Use SNMPv3 to talk to the experimental server */

    /* set the SNMP version number */
    session.version = SNMP_VERSION_3;

    /* set the SNMPv3 user name */
    session.securityName = strdup(snmpusername);
    session.securityNameLen = strlen(session.securityName);

    /* set the security level to authenticated and encrypted */
    session.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;

    /* set the authentication method to SHA */
    session.securityAuthProto = usmHMACSHA1AuthProtocol;
    session.securityAuthProtoLen = USM_AUTH_PROTO_SHA_LEN;
    session.securityAuthKeyLen = USM_AUTH_KU_LEN;

    /* set the authentication key to a SHA1 hashed version of our
       passphrase "SNMPVeicular" (which must be at least 8
       characters long) */
    if (generate_Ku(session.securityAuthProto,
                    session.securityAuthProtoLen,
                    (u_char *)our_v3_passphrase, strlen(our_v3_passphrase),
                    session.securityAuthKey,
                    &session.securityAuthKeyLen) != SNMPERR_SUCCESS)
    {
        snmp_perror(argv[0]);
        snmp_log(LOG_ERR,
                 "Error generating Ku from authentication pass phrase. \n");
        exit(1);
    }
    /* set the encryption method to AES */
    session.securityPrivProto = snmp_duplicate_objid(usmAESPrivProtocol, USM_PRIV_PROTO_AES_LEN);
    session.securityPrivProtoLen = USM_PRIV_PROTO_AES_LEN;
    session.securityPrivKeyLen = USM_PRIV_KU_LEN;
    if (generate_Ku(session.securityAuthProto,
                    session.securityAuthProtoLen,
                    (u_char *)our_v3_encryptionphrase, strlen(our_v3_encryptionphrase),
                    session.securityPrivKey,
                    &session.securityPrivKeyLen) != SNMPERR_SUCCESS)
    {
        snmp_perror(argv[0]);
        snmp_log(LOG_ERR,
                 "Error generating Ku from authentication pass phrase. \n");
        exit(1);
    }
    /*
     * Open the session
     */
    SOCK_STARTUP;
    ss = snmp_open(&session); /* establish the session */

    if (!ss)
    {
        snmp_sess_perror("ack", &session);
        SOCK_CLEANUP;
        exit(1);
    }
    int keep_running = 1;
    while (keep_running)
    {
        int escolha = -1;
        printMenu();
        int aux = scanf("%d", &escolha);
        int res;
        /*Set saving mode to 1*/
        char *modOidString[] = {"SavingMode.0"};
        char *types = "i";
        char *values[] = {"1"};
        oid modOidList[] = {1, 3, 6, 1, 3, 8888, 1, 1, 5, 0};
        switch (escolha)
        {
        case 0:
            keep_running = 0;
            break;
        case 1:
            printf("\nValores requestMonitoringDataTable\n");
            res = bulkget(session, ss, requestMonitoringDataTableOid, OID_LENGTH(requestMonitoringDataTableOid));
            printf("\n\n");
            break;
        case 2:
            printf("\nValores requestControlDataTable\n");
            res = bulkget(session, ss, requestControlDataTableOid, OID_LENGTH(requestControlDataTableOid));
            printf("\n\n");
            break;
        case 3:
            printf("\nValores requestStatisticsDataTable\n");
            res = bulkget(session, ss, requestStatisticsDataTableOid, OID_LENGTH(requestStatisticsDataTableOid));
            printf("\n\n");
            break;
        case 4:
            printf("\nValores capabilitiesTable\n");
            res = bulkget(session, ss, capabilitiesTableOid, OID_LENGTH(capabilitiesTableOid));
            printf("\n\n");
            break;
        case 5:
            printf("\nValores samplesTable\n");
            res = bulkget(session, ss, samplesTableOid, OID_LENGTH(samplesTableOid));
            printf("\n\n");
            break;
        case 6:
            printf("\nValores mapTypeTable\n");
            res = bulkget(session, ss, mapTypeTableOid, OID_LENGTH(mapTypeTableOid));
            printf("\n\n");
            break;
        case 7:
            printf("\nValores genericTypesTable\n");
            res = bulkget(session, ss, genericTypesTableOid, OID_LENGTH(genericTypesTableOid));
            printf("\n\n");
            break;
        case 8:
            printf("\nValores sampleUnitsTable\n");
            res = bulkget(session, ss, sampleUnitsTableOid, OID_LENGTH(sampleUnitsTableOid));
            printf("\n\n");
            break;
        case 9:
            printf("\nValores errorTable\n");
            res = bulkget(session, ss, errorTableOid, OID_LENGTH(errorTableOid));
            printf("\n\n");
            break;
        case 10:
            printf("\nValores errorDescriptionTable\n");
            res = bulkget(session, ss, errorDescriptionTableOid, OID_LENGTH(errorDescriptionTableOid));
            printf("\n\n");
            break;
        case 11:
            printf("\nValores commandTemplateTable\n");
            res = bulkget(session, ss, commandTemplateTableOid, OID_LENGTH(commandTemplateTableOid));
            printf("\n\n");
            break;
        case 12:
            printf("\nValores commandTable\n");
            res = bulkget(session, ss, commandTableOid, OID_LENGTH(commandTableOid));
            printf("\n\n");
            break;
        case 13:
            res = set(session, ss, modOidList, OID_LENGTH(modOidList), modOidString, types, values, 1);
            printf("\n\n");
            break;
        default:
            printf("\nInput Inválido\n");
            continue;
        }
    }
    snmp_close(ss);

    SOCK_CLEANUP;
    return (0);
} /* main() */
