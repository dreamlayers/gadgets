#include <stdio.h>
#include "tcp.h"

int tcpc_open(const char *name, unsigned int port)
{
    int nRet;
#ifdef WIN32
    WORD version = MAKEWORD(1,1);
    WSADATA wsaData;
    SOCKET theSocket;
    LPHOSTENT lpHostEntry;
    SOCKADDR_IN saServer;

    /* Start up Winsock */
    WSAStartup(version, &wsaData);
#else
    struct hostent *lpHostEntry;
    int theSocket;
    struct sockaddr_in saServer;
#endif

    /* Store information about the server */
    lpHostEntry = gethostbyname(name);
    if (lpHostEntry == NULL) {
      printf("Error at gethostbyname()");
#ifdef WIN32
      WSACleanup();
#endif
      return -1;
    }

    /* Create the socket */
    theSocket = socket(AF_INET,            /* Go over TCP/IP */
                       SOCK_STREAM,        /* Socket type */
                       IPPROTO_TCP);    /* Protocol */
    if (theSocket == INVALID_SOCKET) {
        printf("Error at socket()");
#ifdef WIN32
      WSACleanup();
#endif
        return -1;
    }


    /* Use SOCKADDR_IN to fill in address information */
    saServer.sin_family = AF_INET;
#ifdef WIN32
    saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
#else
    saServer.sin_addr = *((struct in_addr *)*lpHostEntry->h_addr_list);
#endif
    saServer.sin_port = htons(port);

    /* Connect to the server */
    nRet = connect(theSocket,
               (struct sockaddr *)&saServer,        // Server address
               sizeof(struct sockaddr));    // Length of address structure
        if (nRet == SOCKET_ERROR) {
       /* FIXME:  closesocket ? */
       printf("Error at connect()");
#ifdef WIN32
       WSACleanup();
#endif
       return -1;
    }

    return theSocket;
}

void tcpc_close(int sock)
{
#ifdef WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
}
