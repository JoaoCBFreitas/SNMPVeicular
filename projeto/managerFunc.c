#include "managerFunc.h"
/*This function the menu that so the user can choose which tables to view*/
void viewTablesMenu()
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

/*This function will append vars to linked list tc*/
void appendContent(table_contents **tc, netsnmp_variable_list *vars)
{
    table_contents *newNode = (table_contents *)malloc(sizeof(table_contents));
    table_contents *last = *tc;
    newNode->data = vars;
    newNode->next = NULL;
    if (*tc == NULL)
    {
        *tc = newNode;
        return;
    }
    while (last->next != NULL)
        last = last->next;
    last->next = newNode;
    return;
}

/*This function will iterate through tc and print its contents*/
void printTable(table_contents *tc)
{
    /*
        * check resulting variables 
        */
    while (tc->next != NULL)
    {
        for (; tc->data;
             tc->data = tc->data->next_variable)
        {

            print_variable(tc->data->name, tc->data->name_length, tc->data);
        }
        tc = tc->next;
    }
}

/*This function will send set requests to the agent*/
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

/*This function will send bulkget requests to the agent*/
int bulkget(netsnmp_session session, netsnmp_session *ss, oid *root, size_t rootlen, table_contents **tc)
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
        pdu->max_repetitions = 200; /* fill the packet */
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
                    /*add vars to linked list*/
                    appendContent(tc, vars);

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

void viewTables(netsnmp_session session, netsnmp_session *ss)
{
    int keep_running = 1;
    while (keep_running)
    {
        int escolha = -1;
        viewTablesMenu();
        int aux = scanf("%d", &escolha);
        int res;
        table_contents *tablecontent = NULL;
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
            printf("\nRequestMonitoringDataTable\n");
            res = bulkget(session, ss, requestMonitoringDataTableOid, OID_LENGTH(requestMonitoringDataTableOid), &tablecontent);
            break;

        case 2:
            printf("\nRequestControlDataTable\n");
            res = bulkget(session, ss, requestControlDataTableOid, OID_LENGTH(requestControlDataTableOid), &tablecontent);
            break;
        case 3:
            printf("\nRequestStatisticsDataTable\n");
            res = bulkget(session, ss, requestStatisticsDataTableOid, OID_LENGTH(requestStatisticsDataTableOid), &tablecontent);
            break;
        case 4:
            printf("\nCapabilitiesTable\n");
            res = bulkget(session, ss, capabilitiesTableOid, OID_LENGTH(capabilitiesTableOid), &tablecontent);
            break;
        case 5:
            printf("\nSamplesTable\n");
            res = bulkget(session, ss, samplesTableOid, OID_LENGTH(samplesTableOid), &tablecontent);
            break;
        case 6:
            printf("\nMapTypeTable\n");
            res = bulkget(session, ss, mapTypeTableOid, OID_LENGTH(mapTypeTableOid), &tablecontent);
            break;
        case 7:
            printf("\nGenericTypesTable\n");
            res = bulkget(session, ss, genericTypesTableOid, OID_LENGTH(genericTypesTableOid), &tablecontent);
            break;
        case 8:
            printf("\nSampleUnitsTable\n");
            res = bulkget(session, ss, sampleUnitsTableOid, OID_LENGTH(sampleUnitsTableOid), &tablecontent);
            break;
        case 9:
            printf("\nErrorTable\n");
            res = bulkget(session, ss, errorTableOid, OID_LENGTH(errorTableOid), &tablecontent);
            break;
        case 10:
            printf("\nErrorDescriptionTable\n");
            res = bulkget(session, ss, errorDescriptionTableOid, OID_LENGTH(errorDescriptionTableOid), &tablecontent);
            break;
        case 11:
            printf("\nCommandTemplateTable\n");
            res = bulkget(session, ss, commandTemplateTableOid, OID_LENGTH(commandTemplateTableOid), &tablecontent);
            break;
        case 12:
            printf("\nCommandTable\n");
            res = bulkget(session, ss, commandTableOid, OID_LENGTH(commandTableOid), &tablecontent);
            break;
        case 13:
            res = set(session, ss, modOidList, OID_LENGTH(modOidList), modOidString, types, values, 1);
            printf("\n\n");
            break;
        default:
            printf("\nInvalid Input\n");
            escolha = 0;
            continue;
        }
        if (escolha > 0 && escolha < 13)
            if (tablecontent)
            {
                printTable(tablecontent);
                printf("\n\n");
            }
            else
                printf("Table is Empty\n\n");
        table_contents *tmp;
        while (tablecontent != NULL)
        {
            tmp = tablecontent;
            tablecontent = tablecontent->next;
            free(tmp);
        }
    }
    return;
}

void activeErrors(netsnmp_session session, netsnmp_session *ss)
{
    table_contents *errorDescr = NULL;
    table_contents *error = NULL;
    bulkget(session, ss, errorDescriptionTableOid, OID_LENGTH(errorDescriptionTableOid), &errorDescr);
    bulkget(session, ss, errorTableOid, OID_LENGTH(errorTableOid), &error);
    table_contents *tmp;
    while (errorDescr != NULL)
    {
        tmp = errorDescr;
        errorDescr = errorDescr->next;
        free(tmp);
    }
    while (error != NULL)
    {
        tmp = error;
        error = error->next;
        free(tmp);
    }
}
