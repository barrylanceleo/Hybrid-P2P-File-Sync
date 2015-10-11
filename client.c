//
// Created by barry on 9/29/15.
//
#include "list.h"
#include "socketUtils.h"
#include "client.h"
#include "packetUtils.h"
#include "main.h"
#include "server.h"

fd_set masterFDList, tempFDList; //Master file descriptor list to add all sockets and stdin
int fdmax; //to hold the max file descriptor value
struct host *masterServer; //details about the master server
int connectionId; //index for connections


int runClient(char *port) {
    char *name = "Client";

    //initialize masterServer, peerList and hostlist
    peerList = NULL;
    connectionList = NULL;
    connectionId = 1;
    masterServer = NULL;

    int listernerSockfd = -1;
    struct connectionInfo *serverInfo = startServer(port, "CLIENT");
    if (serverInfo == NULL) {
        fprintf(stderr, "Did not get serverInfo from startServer()\n");
        return -1;
    }
    listernerSockfd = serverInfo->listernerSockfd;

    int STDIN = 0; //0 represents STDIN

    FD_ZERO(&masterFDList); // clear the master and temp sets
    FD_ZERO(&tempFDList);

    FD_SET(STDIN, &masterFDList); // add STDIN to master FD list
    fdmax = STDIN;

    FD_SET(listernerSockfd, &masterFDList); //add the listener to master FD list and update fdmax
    if (listernerSockfd > fdmax)
        fdmax = listernerSockfd;

    while (1) //keep waiting for input, connections and data
    {
        printf("$");
        fflush(stdout); //print the terminal symbol
        tempFDList = masterFDList; //make a copy of masterFDList and use it as select() modifies the list

        //int select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
        if (select(fdmax + 1, &tempFDList, NULL, NULL, NULL) ==
            -1) //select waits till it gets data in an fd in the list
        {
            fprintf(stderr, "Error in select\n");
            return -1;
        }

        // an FD has data so iterate through the list to find the fd with data
        int fd;
        for (fd = 0; fd <= 20; fd++) {
            if (FD_ISSET(fd, &tempFDList)) //found the FD with data
            {
                if (fd == STDIN) //data from commandLine(STDIN)
                {
                    size_t commandLength = 50;
                    char *command = (char *) malloc(commandLength);
                    getline(&command, &commandLength, stdin); //get line the variable if space is not sufficient
                    if (stringEquals(command, "\n")) //to handle the stray \n s
                        continue;
                    //printf("--Got data: %s--\n",command);
                    handleCommands(command, "CLIENT");
                }
                else if (fd == listernerSockfd) //new client trying to connect to listener
                {

                    struct sockaddr_storage clientaddr;

                    socklen_t addrlen = sizeof clientaddr;
                    int clientsockfd;

                    if ((clientsockfd = accept(listernerSockfd, (struct sockaddr *) &clientaddr, &addrlen)) == -1) {
                        fprintf(stderr, "Error accepting connection: %d %s\n", clientsockfd,
                                gai_strerror(clientsockfd));
                        return -1;
                    }
                    else {
                        //accept connection from client add it to the connectionList
                        struct host *client = (struct host *) malloc(sizeof(struct host));
                        struct sockaddr *hostAddress = (struct sockaddr *) &clientaddr;
                        client->id = connectionId++;
                        client->sockfd = clientsockfd;
                        client->ipAddress = getIPAddress(hostAddress);
                        client->hostName = getHostFromIp(client->ipAddress);
                        client->port = getPort(hostAddress);
                        addNode(&connectionList, client);
                        FD_SET(client->sockfd, &masterFDList); // add fd to fdlist
                        if (client->sockfd > fdmax)
                            fdmax = client->sockfd;
                        printf("Accepted client: %s %s on port: %s\n", client->hostName,
                               client->ipAddress, client->port);

                        //send ack to client
                        char *msg = "ACK";
                        int bytes_sent = send(clientsockfd, msg, strlen(msg), 0);
                        printf("ACK sent %d\n", bytes_sent);
                    }

                }
                else if (masterServer != NULL && fd == masterServer->sockfd)// handle data from server
                {
                    char buffer[1000];
                    int bytes_received = recv(fd, buffer, 1000, 0);
                    buffer[bytes_received] = 0;

                    //if the server terminates unexpectedly
                    if (buffer[0] == 0) {
                        printf("Master Server has terminated unexpectedly.\n");
                        peerList = NULL;
                        //add code to remove masterserver from clientList
                        masterServer->sockfd = -1;
                        close(fd);
                        FD_CLR(fd, &masterFDList);
                        continue;
                    }

                    struct packet *recvPacket = packetEncoder(buffer);
                    printPacket(recvPacket);

                    //received terminate from server
                    if (recvPacket->header->messageType == terminate) {
                        printf("Received TERMINATE from server\n");
                        int id = getIDdForFD(connectionList, fd);
                        terminateClient(id);
                        peerList = NULL;
                        continue;
                    }

                    //split the hostlist
                    int length = 0;
                    char **hostinfo = splitString(recvPacket->message, ' ', &length);
                    peerList = NULL;
                    int i;
                    for (i = 0; i < length; i = i + 2) {
                        if (i + 1 >= length)
                            fprintf(stderr, "Disproportionate terms in hostList sent by server.\n");

                        if (stringEquals(myIpAddress, hostinfo[i]) && stringEquals(myListenerPort, port)) {
                            //this is so that the client doesn't add itself in the peerList
                            continue;
                        }

                        //add all nodes
                        struct host *peer = (struct host *) malloc(sizeof(struct host));
                        peer->sockfd = -1; // we do not have a connection with it yet
                        peer->ipAddress = hostinfo[i];
                        peer->hostName = getHostFromIp(peer->ipAddress);
                        peer->port = hostinfo[i + 1];
                        addNode(&peerList, peer);
                        //printf("Added %s %s %s to peerList\n", peer->hostName, peer->ipAddress, peer->port);

//                        //if the host is not present in the list add it
//                        if (!isHostPresent(peerList, hostinfo[i], hostinfo[i + 1])) {
//                            struct host *peer = (struct host *) malloc(sizeof(struct host));
//                            peer->sockfd = -1; // we do not have a connection with it yet
//                            peer->ipAddress = hostinfo[i];
//                            peer->hostName = getHostFromIp(peer->ipAddress);
//                            peer->port = hostinfo[i + 1];
//                            addNode(&peerList, peer);
//                            printf("Added %s %s %s to peerList\n", peer->hostName, peer->ipAddress, peer->port);
//                        }

                    }
                    printf("New peerList received from server:\n");
                    printPeerList(peerList);
                }
                else {
                    //handle data from the peers

                    char buffer[1000];
                    int bytes_received = recv(fd, buffer, 1000, 0);
                    buffer[bytes_received] = 0;
                    //if one of the peers terminates unexpectedly
                    if (buffer[0] == 0) {
                        int id = getIDdForFD(connectionList, fd);
                        struct host *host = getNodeForID(connectionList, id);
                        printf("Peer: %s/%s Sock FD:%d terminated unexpectedly. Removing it from the list.\n",
                               host->ipAddress, host->port, host->sockfd);
                        terminateConnection(id);
                        continue;
                    }
                    else {
                        struct packet *recvPacket = packetEncoder(buffer);
                        printPacket(recvPacket);

                        //received terminate
                        if (recvPacket->header->messageType == terminate)
                            printf("Received TERMINATE\n");
                        int id = getIDdForFD(connectionList, fd);
                        terminateConnection(id);
                        continue;
                    }

                    //other data handle
                }
            }
        }
    }
}

int connectToHost(char *hostName, char *port) //connects to give ipaddress or hostname/portnumber
// adds the sock fd masterFDList and returns the sockfd
{
    // check if getAddressInfo() handles both hostname and ipaddress #needtomodify
    struct addrinfo *serverAddressInfo = getAddressInfo(hostName, port);
    if (serverAddressInfo == NULL) {
        return -1;
    }
    printf("Server IP Address: %s Port: %s\n",
           getIPAddress(serverAddressInfo->ai_addr), getPort(serverAddressInfo->ai_addr));

    int clientSocketfd;

    if ((clientSocketfd = socket(serverAddressInfo->ai_family, serverAddressInfo->ai_socktype,
                                 serverAddressInfo->ai_protocol)) == -1) {
        fprintf(stderr, "Error Creating socket: %d %s\n", clientSocketfd, gai_strerror(clientSocketfd));
        return -1;
    } else {
        printf("Created Socket.\n");
    }

    int connectStatus;

    if ((connectStatus = connect(clientSocketfd, serverAddressInfo->ai_addr, serverAddressInfo->ai_addrlen)) == -1) {
        fprintf(stderr, "Error connecting to host %d %s\n", connectStatus, gai_strerror(connectStatus));
        freeaddrinfo(serverAddressInfo); // free the address list
        return -1;
    }
    else {
        printf("Connected to %s.\n", hostName); //#needtomodify this looks like a hack
        FD_SET(clientSocketfd, &masterFDList); // add clientSocketfd to master FD list
        if (clientSocketfd > fdmax)
            fdmax = clientSocketfd;
        freeaddrinfo(serverAddressInfo); // free the address list
        return clientSocketfd;
    }
}

int registerToServer(char *hostName, char *port) {
    hostName = "localhost"; //for testing on local machine #needtomodify
    printf("Registering to Server %s/%s\n", hostName, port);
    if (masterServer != NULL) {
        printf("The client is already registered to the master server.\n");
        return 0;
    }
    int serversockfd = connectToHost(hostName, port); //connect and get sockfd
    if (serversockfd == -1) {
        fprintf(stderr, "There is no server running on given hostname/port\n");
    }
    else {
        //add server to the connectionList
        masterServer = (struct host *) malloc(sizeof(struct host));
        masterServer->id = connectionId++;
        masterServer->hostName = hostName;
        masterServer->ipAddress = getIpfromHost("127.0.0.1"); // change to hostName
        masterServer->port = port;
        masterServer->sockfd = serversockfd;
        addNode(&connectionList, masterServer);

        //once registered send the listener port of the client to the server
        char *message = myListenerPort;
        struct packet *pckt = packetBuilder(registerHost, NULL, strlen(message), message);
        char *packetString = packetDecoder(pckt);
        int bytes_sent = send(masterServer->sockfd, packetString, strlen(packetString), 0);
        if (bytes_sent != -1) {
            printf("Sent the listerner port %s to the server.\n", message);
        }
        free(pckt);

        //receive ack from the server
        char recvdata[4];
        int bytes_received = recv(masterServer->sockfd, recvdata, 4, 0);
        recvdata[bytes_received] = 0;
        //printf("ACK Packet: %s\n", recvdata);
        struct packet *recvPacket = packetEncoder(recvdata);
        //printPacket(recvPacket);
        if (recvPacket->header->messageType == ack)
            printf("Received ACK\n");
    }
    return 0;
}

int connectToClient(char *hostName, char *port) {

    //Check if hostname/ip and port is registered to server
//    if(!isHostPresent(peerList, "localhost", port))
//    {
//        printf("This client you are trying to connect to is not registered with the server.\n");
//        return -1;
//    }

    //check if hostName given was in fact an ipaddress
    char *tempHostName = getHostFromIp(hostName);
    if (!stringEquals(tempHostName, "invalid Ip")) {
        //hostName is a valid ipaddress
        hostName = tempHostName;
    }

    printf("connecting to Hostname: %s Port: %s\n", hostName, port);

    int clientSockfd = connectToHost("localhost", port); //"localhost" should be hostName #needtomodify
    if (clientSockfd == -1) {
        fprintf(stderr, "Error connecting to client\n");
    }
    else {
        //add the client to the connectionList
        struct host *client = (struct host *) malloc(sizeof(struct host));
        client->id = connectionId++;
        client->sockfd = clientSockfd;
        client->ipAddress = getIpfromHost(hostName);
        client->hostName = hostName;
        client->port = port;
        addNode(&connectionList, client);
        printf("Connected to client: %s on port: %s\n", client->hostName,
               client->port);

        //receive ack from the client #needtomodify
        char buffer[3];
        int bytes_received = recv(clientSockfd, buffer, 3, 0);
        buffer[bytes_received] = 0;
        printf("Received: %s\n", buffer);
    }
    return 0;
}

int printConnectionList(struct list *head) {
    struct list *current = head;
    if (current == NULL) {
        fprintf(stdout, "There are no connections at the moment.\n");
        return 0;
    }
    else {
        printf("id:\t\tHostname\t\tIP Address\t\tPort No.\t\tSock FD\n");
        do {
            struct host *currenthost = (struct host *) current->value;
            printf("%d: \t\t%s\t\t%s\t\t%s\t\t%d\n", currenthost->id, currenthost->hostName,
                   currenthost->ipAddress, currenthost->port, currenthost->sockfd);
            current = current->next;
        } while (current != NULL);
    }
    return 0;
}

int printPeerList(struct list *head) {
    struct list *current = head;
    if (current == NULL) {
        fprintf(stdout, "There are no connections at the moment.\n");
        return 0;
    }
    else {
        printf("Hostname\t\tIP Address\t\tPort No.\n");
        do {
            struct host *currenthost = (struct host *) current->value;
            printf("%s\t\t%s\t\t%s\n", currenthost->hostName,
                   currenthost->ipAddress, currenthost->port);
            current = current->next;
        } while (current != NULL);
    }
    return 0;
}

int terminateConnection(int id) {
    struct host *host = getNodeForID(connectionList, id);
    if (host == NULL) {
        printf("Connection %d not found.\n", id);
        return 0;
    }

    //send terminate message to the corresponding peer
    char *message = "";
    struct packet *pckt = packetBuilder(terminate, NULL, strlen(message), message);
    char *packetString = packetDecoder(pckt);
    printf("TERMINATE Packet: %s\n", packetString);
    int bytes_sent = send(host->sockfd, packetString, strlen(packetString), 0);
    printf("Sent Terminate: %d\n", bytes_sent);

    //close sock and remove it from fdlist and connection list
    close(host->sockfd);
    FD_CLR(host->sockfd, &masterFDList);
    connectionList = removeNodeById(connectionList, id);
    if (connectionList == NULL) {
        printf("Got empty connection list\n");
    }
    printf("CLosed SockFd: %d\n", host->sockfd);
    return 0;
}

void quitClient() {
    if (connectionList == NULL) {
        printf("Not connected to any peer.\n");
        exit(0);
    }

    //Build terminate packet
    char *message = "";
    struct packet *pckt = packetBuilder(terminate, NULL, strlen(message), message);
    char *packetString = packetDecoder(pckt);
    printf("TERMINATE Packet: %s\n", packetString);

    //send terminate message all connected peers
    int bytes_sent;
    struct list *current = connectionList;
    do {
        struct host *currenthost = (struct host *) current->value;
        printf("FD: %d\n", currenthost->sockfd);
        bytes_sent = send(currenthost->sockfd, packetString, strlen(packetString), 0);
        printf("Sent Terminate: %d to FD: %d\n", bytes_sent, currenthost->sockfd);
        close(currenthost->sockfd);
        FD_CLR(currenthost->sockfd, &masterFDList);
        printf("Closed SockFd: %d\n", currenthost->sockfd);
        current = current->next;
    } while (current != NULL);
    free(connectionList);
    exit(0);
}