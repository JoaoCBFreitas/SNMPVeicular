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
    printf("*ErrorTable                 -9 *\n");
    printf("*ErrorDescriptionTable      -10*\n");
    printf("*CommandTemplateTable       -11*\n");
    printf("*CommandTable               -12*\n");
    printf("*Exit                       -0 *\n");
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
/*This function will append an active error node ce to linked list ae*/
void appendErrors(active_errors **ae, active_errors *ce)
{
    active_errors *last = *ae;
    if (*ae == NULL)
    {
        *ae = ce;
        return;
    }
    while (last->next != NULL)
        last = last->next;
    last->next = ce;
    return;
}
/*This function will append an command_template in to linked list ct*/
void appendCommand(command_template **ct, command_template *in)
{
    command_template *last = *ct;
    if (*ct == NULL)
    {
        *ct = in;
        return;
    }
    while (last->next != NULL)
        last = last->next;
    last->next = in;
    return;
}
/*This function will iterate through tc and print its contents*/
void printTable(table_contents *tc)
{
    while (tc)
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
int set(netsnmp_session session, netsnmp_session *ss, oid **root, size_t *rootlen, char **oidString, char *types, char **values, int total_oids)
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
    for (count = 0; count < total_oids; count++)
    {
        get_node(oidString[count], root[count], &rootlen[count]);
        if (snmp_add_var(pdu, root[count], rootlen[count], types[count], values[count]))
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
    active_errors *ae = NULL;
    active_errors *ce;
    /*These configs will make contents of struct error->data more easy to use*/
    int orig_config_val_qp = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT);
    int orig_config_val_bv = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, 1);

    /*Get the contents of error and errorDescr*/
    bulkget(session, ss, errorDescriptionTableOid, OID_LENGTH(errorDescriptionTableOid), &errorDescr);
    bulkget(session, ss, errorTableOid, OID_LENGTH(errorTableOid), &error);
    /*These will be used to store the head of error and errorDescr*/
    table_contents *headError = error;
    table_contents *headErrorDescr = errorDescr;
    printf("\n");
    if (error)
    {
        while (error)
        {
            int currentIndex = error->data->name[error->data->name_length - 1];
            active_errors *head = ae;
            ce = NULL;
            if (ae)
            {
                /*search for node with current Index*/
                while (ae)
                {
                    /*Node found*/
                    if (ae->id == currentIndex)
                    {
                        ce = ae;
                        break;
                    }
                    ae = ae->next;
                }
                ae = head;
            }
            if (!ae || !ce)
            {
                /*Either there's no node in the linked list or node was not found, either way append new node*/
                ce = (active_errors *)malloc(sizeof(active_errors));
                ce->id = currentIndex;
                ce->next = NULL;
                appendErrors(&ae, ce);
            }
            switch (error->data->name[error->data->name_length - 2])
            {
            case 1:
                /*Index*/
                ce->index = malloc(sizeof(char) * 10);
                snprint_value(ce->index, 10, error->data->name, error->data->name_length, error->data);
                strcat(ce->index, "\0");
                break;
            case 2:
                /*TimeStamp*/
                ce->timestamp = malloc(1 + error->data->val_len);
                memcpy(ce->timestamp, error->data->val.string, error->data->val_len);
                ce->timestamp[error->data->val_len] = '\0';
                break;
            case 3:
                /*Obtain errorCode and errorDescription from errorDescriptionTable using errorDescriptionID*/
                ce->indexError = *error->data->val.integer;
                table_contents *headtc = errorDescr;
                while (errorDescr)
                {
                    /*Compare indexError with the index of the entry*/
                    if (ce->indexError == errorDescr->data->name[errorDescr->data->name_length - 1])
                    {
                        /*errorDescr->data->name[errorDescr->data->name_length - 2] equals the column*/
                        if (errorDescr->data->name[errorDescr->data->name_length - 2] == 2)
                        {
                            /*Get errorDescr*/
                            ce->errorDesc = malloc(1 + errorDescr->data->val_len);
                            memcpy(ce->errorDesc, errorDescr->data->val.string, errorDescr->data->val_len);
                            ce->errorDesc[errorDescr->data->val_len] = '\0';
                        }
                        if (errorDescr->data->name[errorDescr->data->name_length - 2] == 3)
                        {
                            /*Get errorCode*/
                            ce->errorCode = malloc(100);
                            snprint_value(ce->errorCode, 100, error->data->name, error->data->name_length, error->data);
                            strcat(ce->errorCode, "\0");
                        }
                    }
                    errorDescr = errorDescr->next;
                }
                /*Go back to head of list*/
                errorDescr = headtc;
                break;
            case 4:
                /*Username*/
                ce->username = malloc(1 + error->data->val_len);
                memcpy(ce->username, error->data->val.string, error->data->val_len);
                ce->username[error->data->val_len] = '\0';
                break;
            default:
                break;
            }
            error = error->next;
        }
    }
    else
        printf("No active errors found\n");

    /*Reset configs to original values*/
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, orig_config_val_qp);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, orig_config_val_bv);
    error = headError;
    errorDescr = headErrorDescr;
    /*tmp and errors will be used to free memory of errorDescr,error and ae*/
    table_contents *tmp;
    active_errors *errors;
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
    while (ae != NULL)
    {
        /*Traverse active_errors, print the contents of every node and then free the memory */
        errors = ae;
        ae = ae->next;
        printf("Error %s=[%s] EC%s[%s] User[%s]\n", errors->index, errors->timestamp, errors->errorCode, errors->errorDesc, errors->username);
        free(errors->index);
        free(errors->timestamp);
        free(errors->errorCode);
        free(errors->errorDesc);
        free(errors->username);
        free(errors);
    }
    return;
}

void sendCommand(netsnmp_session session, netsnmp_session *ss)
{
    /*command will be used to find the ID with which to use set command*/
    table_contents *command = NULL;
    table_contents *commandTemplate = NULL;
    /*These configs will make contents of struct command->data and commandTemplate->data more easy to use*/
    int orig_config_val_qp = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT);
    int orig_config_val_bv = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, 1);
    /*Get the contents of commandTable and commandTemplateTable*/
    bulkget(session, ss, commandTableOid, OID_LENGTH(commandTableOid), &command);
    bulkget(session, ss, commandTemplateTableOid, OID_LENGTH(commandTemplateTableOid), &commandTemplate);
    /*Reset configs to original values*/
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, orig_config_val_qp);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, orig_config_val_bv);
    /*These will be used to store the heads of commandTemplate and command*/
    table_contents *headCT = commandTemplate;
    table_contents *headC = command;
    printf("\n");
    /*Print Available Commands on commandTemplateTable*/
    command_template *ae = NULL;
    command_template *ct;
    if (commandTemplate)
    {
        while (commandTemplate)
        {
            int currentIndex = commandTemplate->data->name[commandTemplate->data->name_length - 1];
            command_template *head = ae;
            ct = NULL;
            if (ae)
            {
                /*search for node with current Index*/
                while (ae)
                {
                    /*Node found*/
                    if (ae->id == currentIndex)
                    {
                        ct = ae;
                        break;
                    }
                    ae = ae->next;
                }
                ae = head;
            }
            if (!ae || !ct)
            {
                /*Either there's no node in the linked list or node was not found, either way append new node*/
                ct = (command_template *)malloc(sizeof(command_template));
                ct->id = currentIndex;
                ct->next = NULL;
                appendCommand(&ae, ct);
            }
            switch (commandTemplate->data->name[commandTemplate->data->name_length - 2])
            {
            case 1:
                /*Index*/
                ct->id = *commandTemplate->data->val.integer;
                break;
            case 2:
                /*Description*/
                ct->descr = malloc(1 + commandTemplate->data->val_len);
                memcpy(ct->descr, commandTemplate->data->val.string, commandTemplate->data->val_len);
                ct->descr[commandTemplate->data->val_len] = '\0';
                break;
            case 3:
                /*targetNode*/
                ct->target = malloc(1 + commandTemplate->data->val_len);
                memcpy(ct->target, commandTemplate->data->val.string, commandTemplate->data->val_len);
                ct->target[commandTemplate->data->val_len] = '\0';
                break;
            case 4:
                /*commandTemplate*/
                break;
            default:
                break;
            }
            commandTemplate = commandTemplate->next;
        }
    }
    command_template *com;
    while (ae != NULL)
    {
        /*Traverse command_template, print the contents of every node and then free the memory */
        com = ae;
        ae = ae->next;
        printf("Command %d -[Target=%s] %s\n", com->id, com->target, com->descr);
        free(com->descr);
        free(com->target);
        free(com);
    }
    commandTemplate = headCT;
    command = headC;
    /*tmp will be used to free memory of command and commandTemplate*/
    table_contents *tmp;
    while (command != NULL)
    {
        tmp = command;
        command = command->next;
        free(tmp);
    }
    while (commandTemplate != NULL)
    {
        tmp = commandTemplate;
        commandTemplate = commandTemplate->next;
        free(tmp);
    }
    printf("\n");
    /****************************************** Get User Input ******************************************/
    int templateID;
    int input;
    int aux;
    printf("Choose template to be used:");
    aux = scanf("%d", &templateID);
    printf("Insert Input:");
    aux = scanf("%d", &input);
    /*Convert from integer to string so it can be sent over snmpset*/
    char strTemplate[sizeof(int) * 4 + 1];
    sprintf(strTemplate, "%d", templateID);
    char strInput[sizeof(int) * 4 + 1];
    sprintf(strInput, "%d", input);
    /*Get next free index of commandTable*/
    int commandIndex = 0;
    if (command)
    {
        while (command)
        {
            if (commandIndex <= command->data->name[command->data->name_length - 1])
            {
                commandIndex = command->data->name[command->data->name_length - 1] + 1;
            }
            command = command->next;
        }
    }
    char strIndex[sizeof(int) * 4 + 1];
    sprintf(strIndex, "%d", commandIndex);
    char *values[] = {strTemplate, strInput, "snmpadmin"};
    /****************************************** Send Command ******************************************/
    /*Prep modOidList: copy oidListCommand to it and add the index to every item of the list*/
    oid **modOidList = malloc(CommandNumber * sizeof(oid *));
    for (int i = 0; i < CommandNumber; i++)
    {
        modOidList[i] = malloc(sizeof(oid) * CommandOid);
        for (int j = 0; j < CommandOid; j++)
            modOidList[i][j] = oidListCommand[i][j];
        modOidList[i][CommandOid] = commandIndex;
    }
    /*Prep modOidString: copy oidStringCommand to it and add the index to every item of the list*/
    char **modOidString;
    modOidString = malloc(CommandNumber * sizeof(char *));
    for (int i = 0; i < CommandNumber; i++)
    {
        modOidString[i] = malloc(sizeof(char) * strlen(oidStringCommand[i]) + strlen(strIndex));
        strcpy(modOidString[i], oidStringCommand[i]);
        strcat(modOidString[i], strIndex);
    }
    /*Rootlength is needed, for this table its set as 10 which is the number of sub-ids*/
    size_t *rootlen = malloc(sizeof(size_t *) * CommandNumber);
    for (int i = 0; i < CommandNumber; i++)
    {
        rootlen[i] = CommandOid;
    }
    int res = set(session, ss, modOidList, rootlen, modOidString, typesCommand, values, CommandNumber);
    /*Free alocated memory*/
    free(rootlen);
    for (int i = 0; i < CommandNumber; i++)
    {
        free(modOidString[i]);
        free(modOidList[i]);
    }
    free(modOidString);
    free(modOidList);
    return;
}