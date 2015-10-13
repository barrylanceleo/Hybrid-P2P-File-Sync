//
// Created by barry on 10/4/15.
//

#include "list.h"
#include "socketUtils.h"
#include "packetUtils.h"
#include "server.h"
#include "main.h"

fd_set masterFDList, tempFDList; //Master file descriptor list to add all sockets and stdin
int fdmax; //to hold the max file descriptor value
int connectionIdGenerator; //index for connections
int listernerSockfd; //my listener fd

int broadcastToAllClients(struct list *clientList, char *msg) {
    struct list *current = clientList;
    if (current == NULL) {
        printf("There are no clients registered to broadcast\n");
        return 0;
    }
    else {
        do {
            struct host *currenthost = (struct host *) current->value;
            int bytes_sent = send(currenthost->sockfd, msg, (int) strlen(msg), 0);
            current = current->next;
        } while (current != NULL);
    }
    return 0;
}


int runServer(char *port) {
    //intialize clientList which contains the list of registered clients
    clientList = NULL;
    connectionIdGenerator = 1;
    listernerSockfd = -1;

    struct connectionInfo *serverInfo = startServer(port, "SERVER");
    if (serverInfo == NULL) {
        fprintf(stderr, "Did not get serverInfo from startServer()\n");
        return -1;
    }
    listernerSockfd = serverInfo->listernerSockfd;

//    fd_set masterFDList, tempFDList; //Master file descriptor list to add all sockets and stdin
//    int fdmax; //to hold the max file descriptor value

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
        for (fd = 0; fd <= fdmax; fd++) {
            if (FD_ISSET(fd, &tempFDList)) //found a FD with data
            {
                if (fd == STDIN) //data from commandLine(STDIN)
                {
                    size_t commandLength = 50;
                    char *command = (char *) malloc(commandLength);
                    getline(&command, &commandLength, stdin); //get line the variable if space is not sufficient
                    if (stringEquals(command, "\n")) //to handle the stray \n s
                        continue;
                    //printf("--Got data: %s--\n",command);
                    handleCommands(command, "SERVER");
                }
                else if (fd == listernerSockfd) //new client trying to connect to listener
                {
                    //accept client and broadcast details of all clients to all clients
                    struct sockaddr_storage clientaddr;
                    socklen_t addrlen = sizeof clientaddr;
                    int clientsockfd;
                    if ((clientsockfd = accept(listernerSockfd, (struct sockaddr *) &clientaddr, &addrlen)) == -1) {
                        fprintf(stderr, "Error accepting client: %d %s", clientsockfd, gai_strerror(clientsockfd));
                        return -1;
                    }
                    else {
                        // receive the port sent by the client
                        struct packet *recvPacket = readPacket(clientsockfd);
                        if (recvPacket == NULL) {
                            printf("Recevied zero bytes. Probably because someone terminated.\n");
                            break;
                        }
//                        printf("Received packet: ");
//                        printPacket(recvPacket);
//                        printf("\n");

                        //add the client to the clientList
                        struct host *client = (struct host *) malloc(sizeof(struct host));
                        client->id = connectionIdGenerator++;
                        client->sockfd = clientsockfd;
                        struct sockaddr *hostAddress = (struct sockaddr *) &clientaddr;
                        client->ipAddress = getIPAddress(hostAddress);
                        client->hostName = getHostFromIp(client->ipAddress);
                        client->port = strdup(recvPacket->message);
                        addNode(&clientList, client);
                        //add client to the master list
                        FD_SET(client->sockfd, &masterFDList);
                        if (client->sockfd > fdmax) {
                            fdmax = client->sockfd;
                        }
                        printf("Registered client: %s %s/%s\n", client->hostName,
                               client->ipAddress, client->port);
                        free(recvPacket);

                        //Broadcast the client list to all the registered clients
                        //build the broadcast message
                        char *message = "";
                        struct list *current = clientList;
                        struct host *currentHost = (struct host *) current->value;
                        message = stringConcat(message, currentHost->ipAddress);
                        message = stringConcat(message, " ");
                        message = stringConcat(message, currentHost->port);
                        current = current->next;
                        while (current != NULL) {
                            currentHost = (struct host *) current->value;
                            message = stringConcat(message, " ");
                            message = stringConcat(message, currentHost->ipAddress);
                            message = stringConcat(message, " ");
                            message = stringConcat(message, currentHost->port);
                            current = current->next;
                        }
                        //build packet
                        //printf("Broadcast Message: %s", message);
                        struct packet *pckt = packetBuilder(hostList, NULL, strlen(message), message);
                        char *packetString = packetDecoder(pckt);
                        //printf("Broadcast packet: %s\n", packetString);
                        broadcastToAllClients(clientList, packetString);
                        //printList(clientList);
                    }
                }
                else // handle data from clients
                {

                    struct packet *recvPacket = readPacket(fd);
                    if (recvPacket == NULL) {
                        //one of the clients terminated unexpectedly
                        int id = getIDForFD(clientList, fd);
                        struct host *host = getNodeByID(clientList, id);
                        printf("Cient: %s/%s Sock FD:%d terminated unexpectedly. Removing it from the list.\n",
                               host->ipAddress, host->port, host->sockfd);
                        terminateClient(id);
                        continue;
                    }
//                    printf("Received packet: ");
//                    printPacket(recvPacket);
//                    printf("\n");

                    //received terminate
                    if (recvPacket->header->messageType == terminate) {
                        printf("Received TERMINATE\n");
                        int id = getIDForFD(clientList, fd);
                        terminateClient(id);
                        continue;
                    }

                    if (recvPacket->header->messageType == put || recvPacket->header->messageType == get) {
                        printf("This is the master server. Only clients can put, get or sync files.\n");
                        continue;
                    }

                    if (recvPacket->header->messageType == syncFiles) {
                        int connectionId = getIDForFD(clientList, fd);
                        printf("Received a sync request from client %d. "
                                       "Broacasting message to all registered clients.\n", connectionId);

                        //build a sync packet
                        struct packet *packet = packetBuilder(syncFiles, NULL, 0, NULL);
                        char *packetString = packetDecoder(packet);
                        //printf("Sync Packet: %s\n", packetString);

                        //send sync message to all clients
                        broadcastToAllClients(clientList, packetString);
                        printf("Sync request sent to all clients.\n");
                        continue;
                    }
                }
            }
        }
    }
}

int printClientList(struct list *head) {
    struct list *current = head;
    if (current == NULL) {
        fprintf(stdout, "There are no clients registered.\n");
        return 0;
    }
    else {
        printf("id:\t\tHostname\t\tIP Address\t\tPort No.\t\tSock FD\n");
        do {
            struct host *currenthost = (struct host *) current->value;
            printf("%d: \t\t%s\t\t%s\t\t%s\t\t%d\n", currenthost->id, currenthost->hostName, currenthost->ipAddress,
                   currenthost->port, currenthost->sockfd);
            current = current->next;
        } while (current != NULL);
    }
    return 0;
}

int terminateClient(int id) {

    struct host *host = getNodeByID(clientList, id);
    if (host == NULL) {
        printf("No client found with given id.\n", id);
        return 0;
    }

    //send terminate message to the corresponding client
    char *message = "";
    struct packet *pckt = packetBuilder(terminate, NULL, strlen(message), message);
    char *packetString = packetDecoder(pckt);
    //printf("TERMINATE Packet: %s\n", packetString);
    int bytes_sent = send(host->sockfd, packetString, strlen(packetString), 0);
    //printf("Sent Terminate: %d\n", bytes_sent);

    //close sock and remove it from fdlist and connection list
    close(host->sockfd);
    FD_CLR(host->sockfd, &masterFDList);
    clientList = removeNodeById(clientList, id);
    if (clientList == NULL) {
        printf("Got empty client list\n");
    }
    // printf("Closed SockFd: %d\n", host->sockfd);
    return 0;
}

void quitServer() {
    if (clientList == NULL) {
        printf("No clients connected.\nQuitting Master Server.\n");
        exit(0);
    }

    //Build terminate packet
    char *message = "";
    struct packet *pckt = packetBuilder(terminate, NULL, strlen(message), message);
    char *packetString = packetDecoder(pckt);
    //printf("TERMINATE Packet: %s\n", packetString);

    //send terminate message all registered cients
    int bytes_sent;
    struct list *current = clientList;
    do {
        struct host *currenthost = (struct host *) current->value;
        //printf("FD: %d\n", currenthost->sockfd);
        bytes_sent = send(currenthost->sockfd, packetString, strlen(packetString), 0);
        //printf("Sent Terminate: %d to FD: %d\n", bytes_sent, currenthost->sockfd);
        close(currenthost->sockfd);
        FD_CLR(currenthost->sockfd, &masterFDList);
        //printf("Closed SockFd: %d\n", currenthost->sockfd);
        current = current->next;
    } while (current != NULL);
    close(listernerSockfd);
    free(clientList);
    printf("Quitting Master Server.\n");
    exit(0);
}