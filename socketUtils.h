//
// Created by barry on 10/9/15.
//

#ifndef PEER_SYNC_SOCKETUTILS_H
#define PEER_SYNC_SOCKETUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/select.h>
#include "stringUtils.h"

extern char *myListenerPort; //the port on which i'm listening on
extern char *myIpAddress; //my ip address
extern char *myHostName; //my hostName
//structure to return the connection info after creating a server
struct connectionInfo {
    int listernerSockfd;
    struct addrinfo *serverAddressInfo;
};

struct host {
    int id;
    int sockfd;
    char *port;
    char *hostName;
    char *ipAddress;
};


//methods declared
struct connectionInfo *startServer(char *port, char *hostType);

char *getIPAddress(struct sockaddr *address);

char *getPort(struct sockaddr *address);

char *getHostFromIp(char *ipAddress);

char *getIpfromHost(char *hostName);

struct addrinfo *getAddressInfo(char *hostName, char *port);

struct packet *readPacket(int sockfd);

#endif //PEER_SYNC_SOCKETUTILS_H
