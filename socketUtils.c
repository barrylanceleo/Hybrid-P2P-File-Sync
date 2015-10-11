//
// Created by barry on 10/9/15.
//

#include "socketUtils.h"
#include "client.h"
#include "packetUtils.h"
#include "server.h"

//global variables for host
char *myListenerPort; //the port on which i'm listening on
char *myIpAddress; //my ip address
char *myHostName; //my hostName


char *getIPAddress(struct sockaddr *address) {
    if (address == NULL) {
        return "invalid address";
    }
    char *IPAddress = "";
    if (address->sa_family == AF_INET) {
        IPAddress = (char *) malloc(INET6_ADDRSTRLEN);
        struct sockaddr_in *IPV4Address = (struct sockaddr_in *) address;
        inet_ntop(IPV4Address->sin_family, &(IPV4Address->sin_addr), IPAddress, INET6_ADDRSTRLEN);
    }
    return IPAddress;
}

char *getPort(struct sockaddr *address) {
    int port;
    if (address->sa_family == AF_INET) {
        struct sockaddr_in *IPV4Address = (struct sockaddr_in *) address;
        port = ntohs(IPV4Address->sin_port);
        char *portString = (char *) malloc(10);
        sprintf(portString, "%d", port);
        return portString;
    }
    fprintf(stderr, "Not IPv4 in getPort()\n");
    return NULL;
}

char *getHostFromIp(char *ipAddress) {
    if (ipAddress == NULL) {
        return "invalid Ip";
    }
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ipAddress, &sa.sin_addr);
    char hostName[NI_MAXHOST];
    int result = getnameinfo((struct sockaddr *) &sa, sizeof(sa), hostName, sizeof(hostName), NULL, 0, 0);
    if (result != 0) {
        fprintf(stderr, "Error in getting hostname from IpAddress%s\n", gai_strerror(result));
        return "invalid Ip";
    }
    //printf("%s\n", hostName);
    return strdup(hostName);
}

char *getIpfromHost(char *hostName) {
    struct addrinfo *sockAddress = getAddressInfo(hostName, NULL);
    if (sockAddress == NULL)
        return "invalid hostname";
    char *ipAddress = getIPAddress(sockAddress->ai_addr);
    //printf("HostName: %s IpAddress: %s", hostName, ipAddress);
    return ipAddress;
}

struct addrinfo *getAddressInfo(char *hostName, char *port) {
    //printf("Getting Address Info of host: %s port: %s\n",hostName, port);
    //hostName = "localhost";
    struct addrinfo hints, *host_info_list;
    memset(&hints, 0, sizeof hints); //clear any hint
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    int result;
    if (stringEquals(hostName, "localhost")) {
        result = getaddrinfo(NULL, port, &hints, &host_info_list);
    }
    else {
        result = getaddrinfo(hostName, port, &hints, &host_info_list);
    }
    if (result != 0 || host_info_list == NULL) {
        fprintf(stderr, "Error Getting AddressInfo: %s\n", gai_strerror(result));
        printf("Unable to get the AddressInfo. Please check the hostname/ipAddress provided.\n");
        return NULL;
    }

//    if(host_info_list->ai_next!=NULL)
//        cerr << "More than one IPv4 addresses returned";

    struct addrinfo *tempAddrInfo = host_info_list;
    return tempAddrInfo;
}

struct connectionInfo *startServer(char *port, char *hostType) {

    struct addrinfo *serverAddressInfo = getAddressInfo("localhost", port); //as server needs to be started on localhost

    //struct sockaddr_in *ipv4Address = (struct sockaddr_in *)serverAddressInfo->ai_addr;
    //printf("IPv4 : %d, Sockt_Type : %d\n",ipv4Address->sin_addr.s_addr,serverAddressInfo->ai_socktype);

    if (serverAddressInfo->ai_next != NULL)
        fprintf(stderr, "More than one IPv4 addresses returned");

    int listernerSockfd;

    if ((listernerSockfd = socket(serverAddressInfo->ai_family, serverAddressInfo->ai_socktype,
                                  serverAddressInfo->ai_protocol)) == -1) {
        fprintf(stderr, "Error Creating socket: %d %s\n", listernerSockfd, gai_strerror(listernerSockfd));
        return NULL;
    } else {
        printf("Created Socket.\n");
    }

    if (stringEquals(hostType,
                     "SERVER")) //binding to port only for server, this is only for testing need to bind for client too in production
    {
        int bindStatus;

        if ((bindStatus = bind(listernerSockfd, serverAddressInfo->ai_addr, serverAddressInfo->ai_addrlen)) == -1) {
            fprintf(stderr, "Error binding %d %s\n", bindStatus, gai_strerror(bindStatus));
            return NULL;
        } else {
            printf("Done binding socket to port %s.\n", port);
        }
    }

    int listenStatus;

    if ((listenStatus = listen(listernerSockfd, 10)) == -1) {
        //10 is the backlog
        fprintf(stderr, "Error listening: %d %s\n", listenStatus, gai_strerror(listenStatus));
        return NULL;
    } else {
        printf("Listening...\n");
    }

    //updating the global variable myListenerPort
    int sockStatus;
    struct sockaddr listenerAddress;
    socklen_t len = sizeof(listenerAddress);
    if ((sockStatus = getsockname(listernerSockfd, &listenerAddress, &len)) != 0) {
        fprintf(stderr, "Unable to find listening port %s\n", gai_strerror(sockStatus));
        return NULL;
    }
    else
        myListenerPort = getPort(&listenerAddress);

    //updating global variable name and myIpAddress
    // getIPAddress(&listenerAddress) returns 0.0.0.0 hence getting ip using the hostname
    myHostName = (char *) malloc(sizeof(char) * 128);
    gethostname(myHostName, sizeof myHostName);
    myIpAddress = getIpfromHost(myHostName);

    //build the serverInfo structure to be retunrned
    struct connectionInfo *serverInfo = (struct connectionInfo *) malloc(sizeof(struct connectionInfo));
    serverInfo->listernerSockfd = listernerSockfd;
    serverInfo->serverAddressInfo = serverAddressInfo;

    return serverInfo;
}

