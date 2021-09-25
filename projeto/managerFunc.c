#include "managerFunc.h"
void fflush_stdin()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}
/*Simple function that checks if a given string is a valid number*/
int isNumber(char s[])
{
    for (int i = 0; s[i] != '\0'; i++)
        if (isdigit(s[i]) == 0)
            return 0;
    return 1;
}
/*This function the menu that so the user can choose which tables to view*/
void viewTablesMenu()
{
    printf("*********************************\n");
    printf("*  1-RequestMonitoringDataTable *\n");
    printf("*  2-RequestControlDataTable    *\n");
    printf("*  3-RequestStatisticsDataTable *\n");
    printf("*  4-CapabilitiesTable          *\n");
    printf("*  5-SamplesTable               *\n");
    printf("*  6-MapTypeTable               *\n");
    printf("*  7-GenericTypesTable          *\n");
    printf("*  8-SampleUnitsTable           *\n");
    printf("*  9-ErrorTable                 *\n");
    printf("* 10-ErrorDescriptionTable      *\n");
    printf("* 11-CommandTemplateTable       *\n");
    printf("* 12-CommandTable               *\n");
    printf("*  0-Exit                       *\n");
    printf("*********************************\n");
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
/*This function will append an map_type node ce to linked list ae*/
void appendMapType(map_type **ae, map_type *ce)
{
    map_type *last = *ae;
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
/*This function will append an active_request node ce to linked list ae*/
void appendRequests(active_requests **ae, active_requests *ce)
{
    active_requests *last = *ae;
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
/*This function will append an edit_active_requests node ce to linked list ae*/
void appendEditRequests(edit_active_requests **ae, edit_active_requests *ce)
{
    edit_active_requests *last = *ae;
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
        for (; tc->data; tc->data = tc->data->next_variable)
            print_variable(tc->data->name, tc->data->name_length, tc->data);
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
    /*These configs will make contents of struct error->data easier to traverse and use, 
    otherwise it might break the switch cases*/
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
            /*Get errorIndex of current entry*/
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
            /*Switch based on the column*/
            switch (error->data->name[error->data->name_length - 2])
            {
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
        printf("Error %d=[%s] EC%s[%s] User[%s]\n", errors->id, errors->timestamp, errors->errorCode, errors->errorDesc, errors->username);
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
    /*These configs will make contents of struct command->data and commandTemplate->data easier to traverse and use, 
    otherwise it might break the switch cases*/
    int orig_config_val_qp = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT);
    int orig_config_val_bv = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, 1);
    /*Get the contents of commandTemplateTable*/
    bulkget(session, ss, commandTemplateTableOid, OID_LENGTH(commandTemplateTableOid), &commandTemplate);
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
            /*Get commandTemplateIndex of current entry*/
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
    /****************************************** Get User Input ******************************************/
    char templateID[3] = {0};
    char input[3] = {0};
    fflush_stdin();
    while (printf("Choose Template: ") && scanf("%2[^\n]%*c", templateID) < 1)
        fflush_stdin();
    if (isNumber(templateID) == 0)
    {
        printf("Please insert an digit\n");
        return;
    }
    while (printf("Insert Input:") && scanf("%2[^\n]%*c", input) < 1)
        fflush_stdin();
    if (isNumber(input) == 0)
    {
        printf("Please insert an digit\n");
        return;
    }
    /*Get next free index of commandTable*/
    bulkget(session, ss, commandTableOid, OID_LENGTH(commandTableOid), &command);
    int commandIndex = 0;
    if (command)
        while (command)
        {
            if (commandIndex <= command->data->name[command->data->name_length - 1])
                commandIndex = command->data->name[command->data->name_length - 1] + 1;
            command = command->next;
        }

    char strIndex[sizeof(int) * 4 + 1];
    sprintf(strIndex, "%d", commandIndex);
    char *values[] = {templateID, input, session.securityName};
    /****************************************** Send Command ******************************************/
    /*Prep modOidList: copy oidListCommand to it and add the index to every item of the list*/
    oid **modOidList = malloc(CommandNumber * sizeof(oid *));
    for (int i = 0; i < CommandNumber; i++)
    {
        modOidList[i] = malloc(sizeof(oid) * CommandOid);
        for (int j = 0; j < CommandOid; j++)
            modOidList[i][j] = oidListCommand[i][j];
        modOidList[i][CommandOid - 1] = commandIndex;
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
    /*Reset configs to original values*/
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, orig_config_val_qp);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, orig_config_val_bv);
    return;
}

void sendRequest(netsnmp_session session, netsnmp_session *ss)
{
    /*requests will be used to find the ID with which to use set command*/
    table_contents *mapType = NULL;
    table_contents *genericTypes = NULL;
    table_contents *requests = NULL;
    map_type *mapList = NULL;
    map_type *currMap;
    /*These configs will make contents of struct mapType->data and genericTypes->data easier to traverse and use, 
    otherwise it might break the switch cases*/
    int orig_config_val_qp = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT);
    int orig_config_val_bv = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, 1);
    /*Get the contents of mapTypeTable and genericTypesTable*/
    bulkget(session, ss, mapTypeTableOid, OID_LENGTH(mapTypeTableOid), &mapType);
    bulkget(session, ss, genericTypesTableOid, OID_LENGTH(genericTypesTableOid), &genericTypes);

    /*These will be used to store the heads of mapType and genericTypes*/
    table_contents *headMT = mapType;
    table_contents *headGT = genericTypes;
    printf("\n");
    /******************Print Signal names, their IDs and description*******************/
    if (mapType)
    {
        while (mapType)
        {
            /*Get mapTypeTableIndex of current entry*/
            int currentIndex = mapType->data->name[mapType->data->name_length - 1];
            map_type *head = mapList;
            currMap = NULL;
            if (mapList)
            {
                /*search for node with current Index*/
                while (mapList)
                {
                    /*Node found*/
                    if (mapList->id == currentIndex)
                    {
                        currMap = mapList;
                        break;
                    }
                    mapList = mapList->next;
                }
                mapList = head;
            }
            if (!mapList || !currMap)
            {
                /*Either there's no node in the linked list or node was not found, either way append new node*/
                currMap = (map_type *)malloc(sizeof(map_type));
                currMap->id = currentIndex;
                currMap->next = NULL;
                appendMapType(&mapList, currMap);
            }
            switch (mapType->data->name[mapType->data->name_length - 2])
            {
            case 3:
                /*Obtain typeDescription description from genericTypeTable using genericMapTypeID*/
                currMap->descriptionID = *mapType->data->val.integer;
                table_contents *headtc = genericTypes;
                while (genericTypes)
                {
                    /*Compare descriptionID with the index of the entry*/
                    if (currMap->descriptionID == genericTypes->data->name[genericTypes->data->name_length - 1])
                        /*genericTypes->data->name[genericTypes->data->name_length - 2] equals the column*/
                        if (genericTypes->data->name[genericTypes->data->name_length - 2] == 2)
                        {
                            /*Get generic type description*/
                            currMap->description = malloc(1 + genericTypes->data->val_len);
                            memcpy(currMap->description, genericTypes->data->val.string, genericTypes->data->val_len);
                            currMap->description[genericTypes->data->val_len] = '\0';
                        }
                    genericTypes = genericTypes->next;
                }
                /*Go back to head of list*/
                genericTypes = headtc;
                break;
            case 8:
                /*DataSource*/
                currMap->name = malloc(1 + mapType->data->val_len);
                memcpy(currMap->name, mapType->data->val.string, mapType->data->val_len);
                currMap->name[mapType->data->val_len] = '\0';
                break;
            default:
                break;
            }
            mapType = mapType->next;
        }
    }
    else
        printf("No active Map Type Entries found\n");
    map_type *mtaux;
    while (mapList != NULL)
    {
        /*Traverse mapList, print the contents of every node and free the memory*/
        mtaux = mapList;
        mapList = mapList->next;
        printf("%d [%s]\n", mtaux->id, mtaux->name);
        printf("\t%s\n", mtaux->description);
        free(mtaux->name);
        free(mtaux->description);
        free(mtaux);
    }
    /*When ECU is chosen, print the mapType id alongisde name and description of its sensors*/
    printf("\n");
    /****************************************** Get User Input ******************************************/
    char requestMapID[9] = {0};
    char statisticsID[2] = {0};
    char savingMode[2] = {0};
    char maxNOfSamples[1024] = {0};
    char loopMode[2] = {0};
    char startTime[9] = {0};
    char waitTime[9] = {0};
    char durationTime[9] = {0};
    char expireTime[9] = {0};
    char user[1024] = {0};
    int aux;
    fflush_stdin();
    while (printf("Choose sensor:") && scanf("%8[^\n]%*c", requestMapID) < 1)
        fflush_stdin();
    if (isNumber(requestMapID) == 0)
    {
        printf("Please insert an digit\n");
        return;
    }
    while (printf("Do you want statistics?(0=No,1=Yes): ") && scanf("%1[^\n]%*c", statisticsID) < 1)
        fflush_stdin();
    if (isNumber(statisticsID) == 0)
    {
        printf("Please insert an digit\n");
        return;
    }
    while (printf("Choose Saving Mode(0=Permanent,1=Volatile): ") && scanf("%1[^\n]%*c", savingMode) < 1)
        fflush_stdin();
    if (isNumber(savingMode) == 0)
    {
        printf("Please insert an digit\n");
        return;
    }
    /*Get time inputs*/
    printf("Please indicate start time in the following format 12:00:00 (empty for current system time): ");
    aux = scanf("%8[^\n]%*c", startTime);
    fflush_stdin();

    printf("Please indicate wait time in the following format 12:00:00 (empty for no waitTime): ");
    aux = scanf("%8[^\n]%*c", waitTime);
    if (strcmp(waitTime, "") == 0)
        strcpy(waitTime, "00:00:00");
    fflush_stdin();

    printf("Please indicate duration time in the following format 12:00:00 (empty for 10 minutes duration): ");
    aux = scanf("%8[^\n]%*c", durationTime);
    if (strcmp(durationTime, "") == 0)
        strcpy(durationTime, "00:10:00");
    fflush_stdin();

    printf("Please indicate expire time in the following format 12:00:00 (empty for 10 minutes expiration): ");
    aux = scanf("%8[^\n]%*c", expireTime);
    if (strcmp(expireTime, "") == 0)
        strcpy(expireTime, "00:10:00");
    fflush_stdin();

    printf("Indicate maximum number of samples to be recorded (default is 50): ");
    aux = scanf("%1023[^\n]%*c", maxNOfSamples);
    if (isNumber(maxNOfSamples) == 0)
    {
        printf("Please insert an digit\n");
        return;
    }
    else if (strcmp(maxNOfSamples, "") == 0)
        strcpy(maxNOfSamples, "50");
    fflush_stdin();

    while (printf("Should this request restart once it's over?(1=Yes,2=No): ") && scanf("%1[^\n]%*c", loopMode) < 1)
        fflush_stdin();
    if (isNumber(loopMode) == 0)
    {
        printf("Please insert an digit\n");
        return;
    }

    printf("Indicate username(Default is manager username): ");
    aux = scanf("%1023[^\n]%*c", user);
    if (strcmp(user, "") == 0)
        strcpy(user, session.securityName);

    /*Get next free index of requestMonitoringDataTable*/
    int requestIndex = 0;
    bulkget(session, ss, requestMonitoringDataTableOid, OID_LENGTH(requestMonitoringDataTableOid), &requests);
    table_contents *headR = requests;
    if (requests)
        while (requests)
        {
            if (requestIndex <= requests->data->name[requests->data->name_length - 1])
                requestIndex = requests->data->name[requests->data->name_length - 1] + 1;
            requests = requests->next;
        }
    char strIndex[sizeof(int) * 4 + 1];
    sprintf(strIndex, "%d", requestIndex);
    char *values[] = {requestMapID, statisticsID, savingMode, startTime, waitTime, durationTime, expireTime, maxNOfSamples, loopMode, user};
    /****************************************** Send Request ******************************************/
    /*Prep modOidList: copy oidListRequest to it and add the index to every item of the list*/
    oid **modOidList = malloc(RequestNumber * sizeof(oid *));
    for (int i = 0; i < RequestNumber; i++)
    {
        modOidList[i] = malloc(sizeof(oid) * RequestOid);
        for (int j = 0; j < RequestOid; j++)
            modOidList[i][j] = oidListRequest[i][j];
        modOidList[i][RequestOid - 1] = requestIndex;
    }
    /*Prep modOidString: copy oidStringRequest to it and add the index to every item of the list*/
    char **modOidString;
    modOidString = malloc(RequestNumber * sizeof(char *));
    for (int i = 0; i < RequestNumber; i++)
    {
        modOidString[i] = malloc(sizeof(char) * strlen(oidStringRequest[i]) + strlen(strIndex));
        strcpy(modOidString[i], oidStringRequest[i]);
        strcat(modOidString[i], strIndex);
    }
    /*Rootlength is needed, for this table its set as 10 which is the number of sub-ids*/
    size_t *rootlen = malloc(sizeof(size_t *) * RequestNumber);
    for (int i = 0; i < RequestNumber; i++)
        rootlen[i] = RequestOid;
    int res = set(session, ss, modOidList, rootlen, modOidString, typesRequest, values, RequestNumber);
    /****************************************** Free Allocated Memory ******************************************/
    free(rootlen);
    for (int i = 0; i < RequestNumber; i++)
    {
        free(modOidString[i]);
        free(modOidList[i]);
    }
    free(modOidString);
    free(modOidList);
    mapType = headMT;
    genericTypes = headGT;
    requests = headR;
    /*tmp will be used to free memory of mapType, genericTypes and requests*/
    table_contents *tmp;
    while (requests != NULL)
    {
        tmp = requests;
        requests = requests->next;
        free(tmp);
    }
    while (mapType != NULL)
    {
        tmp = mapType;
        mapType = mapType->next;
        free(tmp);
    }
    while (genericTypes != NULL)
    {
        tmp = genericTypes;
        genericTypes = genericTypes->next;
        free(tmp);
    }
    /*Reset configs to original values*/
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, orig_config_val_qp);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, orig_config_val_bv);
    return;
}

void viewRequests(netsnmp_session session, netsnmp_session *ss)
{
    /*requests will be used to find the ID with which to use set command*/
    table_contents *reqMonitor = NULL;
    table_contents *samples = NULL;
    table_contents *maptype = NULL;
    table_contents *statistics = NULL;
    table_contents *sampleUnits = NULL;
    active_requests *ar = NULL;
    active_requests *cr;
    /*These configs will make contents of struct reqMonitor->data easier to traverse and use, 
    otherwise it might break the switch cases*/
    int orig_config_val_qp = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT);
    int orig_config_val_bv = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, 1);
    /*Get the contents of requestMonitoringDataTable, mapTypeTable and sampleUnitsTable*/
    bulkget(session, ss, requestMonitoringDataTableOid, OID_LENGTH(requestMonitoringDataTableOid), &reqMonitor);
    bulkget(session, ss, mapTypeTableOid, OID_LENGTH(mapTypeTableOid), &maptype);
    bulkget(session, ss, sampleUnitsTableOid, OID_LENGTH(sampleUnitsTableOid), &sampleUnits);

    /*These will be used to store the heads of reqMonitor, mapType and sampleUnits*/
    table_contents *headRE = reqMonitor;
    table_contents *headSU = sampleUnits;
    table_contents *headMT = maptype;
    printf("\n");
    /******************Print Signal names, their IDs and description*******************/
    if (reqMonitor)
    {
        while (reqMonitor)
        {
            /*Get requestMonitoringIndex of current entry*/
            int currentIndex = reqMonitor->data->name[reqMonitor->data->name_length - 1];
            active_requests *head = ar;
            cr = NULL;
            if (ar)
            {
                /*search for node with current Index*/
                while (ar)
                {
                    /*Node found*/
                    if (ar->id == currentIndex)
                    {
                        cr = ar;
                        break;
                    }
                    ar = ar->next;
                }
                ar = head;
            }
            if (!ar || !cr)
            {
                /*Either there's no node in the linked list or node was not found, either way append new node*/
                cr = (active_requests *)malloc(sizeof(active_requests));
                cr->id = currentIndex;
                cr->next = NULL;
                appendRequests(&ar, cr);
            }
            switch (reqMonitor->data->name[reqMonitor->data->name_length - 2])
            {
            case 3:
                /*requestMapID && signal && sampleUnit*/
                cr->mapTypeID = *reqMonitor->data->val.integer;
                table_contents *headtc = maptype;
                while (maptype)
                {
                    /*Compare index with the index of the entry*/
                    if (cr->mapTypeID == maptype->data->name[maptype->data->name_length - 1])
                    {
                        /*maptype->data->name[maptype->data->name_length - 2] equals the column*/
                        if (maptype->data->name[maptype->data->name_length - 2] == 4)
                            /*Get sampleUnitMapID*/
                            cr->sampleUnitID = *maptype->data->val.integer;
                        if (maptype->data->name[maptype->data->name_length - 2] == 8)
                        {
                            /*Get dataSource*/
                            cr->signal = malloc(1 + maptype->data->val_len);
                            memcpy(cr->signal, maptype->data->val.string, maptype->data->val_len);
                            cr->signal[maptype->data->val_len] = '\0';
                        }
                    }
                    maptype = maptype->next;
                }
                /*Go back to head of list*/
                maptype = headtc;
                /*Get sample unit*/
                headtc = sampleUnits;
                while (sampleUnits)
                {
                    if (cr->sampleUnitID == sampleUnits->data->name[sampleUnits->data->name_length - 1])
                        if (sampleUnits->data->name[sampleUnits->data->name_length - 2] == 2)
                        {
                            /*Get unitDescription*/
                            cr->unit = malloc(1 + sampleUnits->data->val_len);
                            memcpy(cr->unit, sampleUnits->data->val.string, sampleUnits->data->val_len);
                            cr->unit[sampleUnits->data->val_len] = '\0';
                        }
                    sampleUnits = sampleUnits->next;
                }
                sampleUnits = headtc;
                break;
            case 4:
                /*requestStatisticsID*/
                cr->statisticsID = *reqMonitor->data->val.integer;
                break;
            case 13:
                /*lastsampleID*/
                cr->lastSampleID = *reqMonitor->data->val.integer;
                break;
            case 14:
                /*Number of Samples*/
                cr->nOfSamples = *reqMonitor->data->val.integer;
                break;
            case 18:
                /*Request User*/
                cr->username = malloc(1 + reqMonitor->data->val_len);
                memcpy(cr->username, reqMonitor->data->val.string, reqMonitor->data->val_len);
                cr->username[reqMonitor->data->val_len] = '\0';
            default:
                break;
            }
            reqMonitor = reqMonitor->next;
        }
    }
    else
        printf("No active Requests found\n");
    printf("\n");
    if (ar)
    {
        /*Print active requests*/
        active_requests *aux;
        active_requests *head = ar;
        chosen_request *chr = NULL;
        printf("ID->SignalName [Number of Samples] Username\n");
        while (ar != NULL)
        {
            aux = ar;
            ar = ar->next;
            printf("\t%d->%s [%d] %s\n", aux->id, aux->signal, aux->nOfSamples, aux->username);
        }
        ar = head;
        /*Choose request*/
        char choice[4] = {0};

        fflush_stdin();
        while (printf("\nChoose request:") && scanf("%3[^\n]%*c", choice) < 1)
            fflush_stdin();
        if (isNumber(choice) == 0)
        {
            printf("Please insert an digit\n");
            return;
        }
        /*Check if chosen request actually exists*/
        while (ar != NULL)
        {
            if (atoi(choice) == ar->id)
            {
                chr = (chosen_request *)malloc(sizeof(chosen_request));
                chr->id = ar->id;
                chr->unit = malloc(sizeof(char) * strlen(ar->unit));
                strcpy(chr->unit, ar->unit);
                chr->statistics = ar->statisticsID;
                chr->avg = 0;
                chr->max = 0;
                chr->min = 0;
                chr->samples = malloc(sizeof(int) * ar->nOfSamples);
                chr->timestamp = malloc(sizeof(char *) * ar->nOfSamples);
                chr->checksum = malloc(sizeof(char *) * ar->nOfSamples);
                chr->username = malloc(sizeof(char) * strlen(ar->username));
                strcpy(chr->username, ar->username);
                chr->signal = malloc(sizeof(char) * strlen(ar->signal));
                strcpy(chr->signal, ar->signal);
                chr->nOfSamples = ar->nOfSamples;
                /*Get statistics, if it exists*/
                if (chr->statistics != 0)
                {
                    bulkget(session, ss, requestStatisticsDataTableOid, OID_LENGTH(requestStatisticsDataTableOid), &statistics);
                    table_contents *headST = statistics;
                    if (statistics)
                        while (statistics)
                        {
                            if (chr->statistics == statistics->data->name[statistics->data->name_length - 1])
                            {
                                /*statistics->data->name[statistics->data->name_length - 2] equals the column*/
                                if (statistics->data->name[statistics->data->name_length - 2] == 4)
                                    /*Get minValue*/
                                    chr->min = *statistics->data->val.integer;
                                if (statistics->data->name[statistics->data->name_length - 2] == 5)
                                    /*Get maxValue*/
                                    chr->max = *statistics->data->val.integer;
                                if (statistics->data->name[statistics->data->name_length - 2] == 6)
                                    /*Get average Value*/
                                    chr->avg = *statistics->data->val.integer;
                            }
                            statistics = statistics->next;
                        }
                    statistics = headST;
                }
                /*Get Samples*/
                bulkget(session, ss, samplesTableOid, OID_LENGTH(samplesTableOid), &samples);
                table_contents *headSA = samples;
                if (samples)
                {
                    int count = 0;
                    int sampleID = ar->lastSampleID;
                    int auxID;
                    while (count < ar->nOfSamples)
                    {
                        /*We will need to traverse samples N times since it's a linked list and it will start from the back*/
                        while (samples || sampleID != 0)
                        {
                            if (sampleID == samples->data->name[samples->data->name_length - 1])
                            {
                                if (samples->data->name[samples->data->name_length - 2] == 3)
                                {
                                    /*Get timestamps*/
                                    chr->timestamp[count] = malloc(sizeof(char) * samples->data->val_len);
                                    strcpy(chr->timestamp[count], samples->data->val.string);
                                }
                                if (samples->data->name[samples->data->name_length - 2] == 5)
                                    /*Get previousSampleID*/
                                    auxID = *samples->data->val.integer;
                                if (samples->data->name[samples->data->name_length - 2] == 7)
                                    /*Get recordedValue*/
                                    chr->samples[count] = *samples->data->val.integer;
                                if (samples->data->name[samples->data->name_length - 2] == 9)
                                {
                                    /*Get checksum*/
                                    chr->checksum[count] = malloc(sizeof(char) * samples->data->val_len);
                                    strcpy(chr->checksum[count], samples->data->val.string);
                                    sampleID = auxID;
                                    break;
                                }
                            }
                            samples = samples->next;
                        }
                        count++;
                        samples = headSA;
                    }
                }
                samples = headSA;
                break;
            }
            ar = ar->next;
        }
        ar = head;
        /*Free memory of ar*/
        while (ar != NULL)
        {
            aux = ar;
            ar = ar->next;
            free(aux->unit);
            free(aux);
        }
        /*Print request contents*/
        if (chr)
        {
            printf("Request %d made by user \"%s\" on %s\n", chr->id, chr->username, chr->signal);
            for (int i = 0; i < chr->nOfSamples; i++)
            {
                printf("Sample %d: %d %s [%s] {%s}\n", chr->nOfSamples - i, chr->samples[i], chr->unit, chr->timestamp[i], chr->checksum[i]);
                free(chr->timestamp[i]);
                free(chr->checksum[i]);
            }
            if (chr->statistics != 0)
                printf("Statistics- MIN(%d) MAX(%d) AVERAGE(%d)\n", chr->min, chr->max, chr->avg);
            free(chr->signal);
            free(chr->unit);
            free(chr->username);
            free(chr->samples);
            free(chr->timestamp);
            free(chr->checksum);
            free(chr);
        }
        else
            printf("Chosen request %s was not found\n", choice);
    }
    reqMonitor = headRE;
    sampleUnits = headSU;
    maptype = headMT;
    /*tmp will be used to free memory of reqMonitor,sampleUnits,statistics and samples*/
    table_contents *tmp;
    while (sampleUnits != NULL)
    {
        tmp = sampleUnits;
        sampleUnits = sampleUnits->next;
        free(tmp);
    }
    while (maptype != NULL)
    {
        tmp = maptype;
        maptype = maptype->next;
        free(tmp);
    }
    while (reqMonitor != NULL)
    {
        tmp = reqMonitor;
        reqMonitor = reqMonitor->next;
        free(tmp);
    }
    while (statistics != NULL)
    {
        tmp = statistics;
        statistics = statistics->next;
        free(tmp);
    }
    while (samples != NULL)
    {
        tmp = samples;
        samples = samples->next;
        free(tmp);
    }
    /*Reset configs to original values*/
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, orig_config_val_qp);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, orig_config_val_bv);
    return;
}

/*This function will allow a user to choose what column in a certain requestID to change and will send the set command accordingly,
thus allowing an user to edit requests*/
void changeRequest(netsnmp_session session, netsnmp_session *ss, char *requestID)
{
    /*User chooses what to change*/
    int keep_running = 1;
    int escolha = -1;
    while (keep_running)
    {
        printf("**********************************\n");
        printf("* 1-Change Saving Mode           *\n");
        printf("* 2-Change Max Number of Samples *\n");
        printf("* 3-Change Loop Mode             *\n");
        printf("* 4-Change Status                *\n");
        printf("* 0-Exit                         *\n");
        printf("**********************************\n");
        int aux = scanf("%d", &escolha);
        if (escolha >= 0 && escolha < 5)
            keep_running = 0;
    }
    /*When ECU is chosen, print the mapType id alongisde name and description of its sensors*/
    printf("\n");
    /****************************************** Get User Input ******************************************/
    switch (escolha)
    {
    case 1:
        printf("0-Permanent Saving Mode\n");
        printf("1-Volatile Saving Mode\n");
        break;
    case 3:
        printf("1-Loop Mode Enabled\n");
        printf("2-Loop Mode Disabled\n");
        break;
    case 4:
        printf("0-Request Off\n");
        printf("1-Request On\n");
        printf("2-Request Set\n");
        printf("3-Request Delete\n");
        printf("4-Request Ready\n");
        break;
    case 0:
        return;
    }
    printf("\n");
    char input[10] = {0};
    fflush_stdin();
    while (printf("Add new Value:") && scanf("%9[^\n]%*c", input) < 1)
        fflush_stdin();
    if (isNumber(input) == 0)
    {
        printf("Please insert an digit\n");
        return;
    }
    char **values = malloc(sizeof(char *));
    values[0] = input;
    size_t *rootlen = malloc(sizeof(size_t *));
    rootlen[0] = EditOid;
    char *types = malloc(sizeof(char));
    types[0] = typesEditRequest[escolha - 1];
    /****************************************** Send Request ******************************************/
    /*Prep modOidList: copy oidListRequest to it and add the index to every item of the list*/
    oid **modOidList = malloc(sizeof(oid *));
    modOidList[0] = malloc(sizeof(oid) * EditOid);
    for (int j = 0; j < EditOid; j++)
        modOidList[0][j] = oidListEditRequest[escolha - 1][j];
    modOidList[0][EditOid - 1] = atoi(requestID);
    /*Prep modOidString: copy oidStringRequest to it and add the index to every item of the list*/
    char **modOidString;
    modOidString = malloc(sizeof(char *));
    modOidString[0] = malloc(sizeof(char) * strlen(oidStringEditRequest[escolha - 1]) + strlen(requestID));
    strcpy(modOidString[0], oidStringEditRequest[escolha - 1]);
    strcat(modOidString[0], requestID);
    int res = set(session, ss, modOidList, rootlen, modOidString, types, values, 1);
    /****************************************** Free Allocated Memory ******************************************/
    free(modOidList[0]);
    free(modOidList);
    free(modOidString[0]);
    free(modOidString);
    free(rootlen);
    free(values);
    free(types);
    return;
}

void editRequests(netsnmp_session session, netsnmp_session *ss)
{
    /*requests will be used to find the ID with which to use set command*/
    table_contents *reqMonitor = NULL;
    edit_active_requests *ar = NULL;
    edit_active_requests *cr;
    /*These configs will make contents of struct reqMonitor->data easier to traverse and use, 
    otherwise it might break the switch cases*/
    int orig_config_val_qp = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT);
    int orig_config_val_bv = netsnmp_ds_get_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, 1);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, 1);
    /*Get the contents of requestMonitoringDataTable, mapTypeTable and sampleUnitsTable*/
    bulkget(session, ss, requestMonitoringDataTableOid, OID_LENGTH(requestMonitoringDataTableOid), &reqMonitor);
    /*This will be used to store the heads of reqMonitor*/
    table_contents *headRE = reqMonitor;
    printf("\n");
    /******************Print Signal names, their IDs and description*******************/
    if (reqMonitor)
        while (reqMonitor)
        {
            /*Get requestMonitoringIndex of current entry*/
            int currentIndex = reqMonitor->data->name[reqMonitor->data->name_length - 1];
            edit_active_requests *head = ar;
            cr = NULL;
            if (ar)
            {
                /*search for node with current Index*/
                while (ar)
                {
                    /*Node found*/
                    if (ar->id == currentIndex)
                    {
                        cr = ar;
                        break;
                    }
                    ar = ar->next;
                }
                ar = head;
            }
            if (!ar || !cr)
            {
                /*Either there's no node in the linked list or node was not found, either way append new node*/
                cr = (edit_active_requests *)malloc(sizeof(edit_active_requests));
                cr->id = currentIndex;
                cr->next = NULL;
                appendEditRequests(&ar, cr);
            }
            switch (reqMonitor->data->name[reqMonitor->data->name_length - 2])
            {
            case 5:
                /*savingMode*/
                cr->savingMode = *reqMonitor->data->val.integer;
                break;
            case 15:
                /*maxNofSamples*/
                cr->maxNOfSamples = *reqMonitor->data->val.integer;
                break;
            case 16:
                /*loopMode*/
                cr->loopMode = *reqMonitor->data->val.integer;
                break;
            case 17:
                /*status*/
                cr->status = *reqMonitor->data->val.integer;
                break;
            case 18:
                /*Request User*/
                cr->username = malloc(1 + reqMonitor->data->val_len);
                memcpy(cr->username, reqMonitor->data->val.string, reqMonitor->data->val_len);
                cr->username[reqMonitor->data->val_len] = '\0';
            default:
                break;
            }
            reqMonitor = reqMonitor->next;
        }
    else
        printf("No active Requests found\n");
    printf("\n");
    if (ar)
    {
        /*Print active requests*/
        edit_active_requests *aux;
        edit_active_requests *head = ar;
        printf("ID->Username {SavingMode} [MaxNOfSamples] {LoopMode} [Status]\n");
        while (ar != NULL)
        {
            aux = ar;
            ar = ar->next;
            printf("\t%d->%s {%d} [%d] {%d} [%d]\n", aux->id, aux->username, aux->savingMode, aux->maxNOfSamples, aux->loopMode, aux->status);
        }
        ar = head;
        /*Choose request*/
        char choice[4] = {0};
        fflush_stdin();
        while (printf("\nChoose request:") && scanf("%3[^\n]%*c", choice) < 1)
            fflush_stdin();
        if (isNumber(choice) == 0)
        {
            printf("Please insert an digit\n");
            return;
        }
        /*Check if chosen request actually exists*/
        while (ar != NULL)
        {
            if (atoi(choice) == ar->id)
            {
                if (strcmp(session.securityName, ar->username) == 0)
                    changeRequest(session, ss, choice);
                else
                    printf("You can't edit requests made by other users\n");
                break;
            }
            ar = ar->next;
        }
        ar = head;
        /*Free memory of ar*/
        while (ar != NULL)
        {
            aux = ar;
            ar = ar->next;
            free(aux->username);
            free(aux);
        }
    }
    reqMonitor = headRE;
    /*tmp will be used to free memory of reqMonitor,sampleUnits,statistics and samples*/
    table_contents *tmp;
    while (reqMonitor != NULL)
    {
        tmp = reqMonitor;
        reqMonitor = reqMonitor->next;
        free(tmp);
    }
    /*Reset configs to original values*/
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_QUICK_PRINT, orig_config_val_qp);
    netsnmp_ds_set_boolean(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_PRINT_BARE_VALUE, orig_config_val_bv);
    return;
}