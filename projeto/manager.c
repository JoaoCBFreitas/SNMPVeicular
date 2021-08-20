#include <manager.h>
/*This function prints the main menu*/
void printMenu()
{
    printf("***********************\n");
    printf("*Create New Request -1*\n");
    printf("*View Requests      -2*\n");
    printf("*View Tables        -3*\n");
    printf("*View Active Errors -4*\n");
    printf("*Send Command       -5*\n");
    printf("*Exit               -0*\n");
    printf("***********************\n");
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
        switch (escolha)
        {
        case 0:
            keep_running = 0;
            break;
        case 1:
            /*Create new Request*/
            printf("\nTODO\n");
            printf("\n\n");
            break;
        case 2:
            /*View Requests*/
            printf("\nTODO\n");
            printf("\n\n");
            break;
        case 3:
            /*View Tables*/
            viewTables(session, ss);
            printf("\n\n");
            break;
        case 4:
            /*View Active Errors*/
            activeErrors(session, ss);
            printf("\n\n");
            break;
        case 5:
            /*Send Command*/
            sendCommand(session, ss);
            printf("\n\n");
            break;
        default:
            printf("\nInvalid Input\n");
            continue;
        }
    }
    snmp_close(ss);

    SOCK_CLEANUP;
    return (0);
} /* main() */
