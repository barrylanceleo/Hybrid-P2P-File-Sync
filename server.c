//
// Created by barry on 10/4/15.
//

#include "list.h"
#include "socketUtils.h"
#include "packetUtils.h"

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
    struct list *clientList = NULL; //intialize clientList which contains the lst of registered clients
    int listernerSockfd = -1;

    struct connectionInfo *serverInfo = startServer(port, "SERVER");
    if (serverInfo == NULL) {
        fprintf(stderr, "Did not get serverInfo from startServer()\n");
        return -1;
    }
    listernerSockfd = serverInfo->listernerSockfd;

    fd_set masterFDList, tempFDList; //Master file descriptor list to add all sockets and stdin
    int fdmax; //to hold the max file descriptor value
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
                        char recvdata[20];
                        int bytes_received = recv(clientsockfd, recvdata, 20, 0);
                        recvdata[bytes_received] = 0;
                        struct packet *recvPacket = packetEncoder(recvdata);

                        //printf("%s\n",recvPacket->message);

                        //add the client to the clientList
                        struct host *client = (struct host *) malloc(sizeof(struct host));
                        client->sockfd = clientsockfd;
                        struct sockaddr *hostAddress = (struct sockaddr *) &clientaddr;
                        client->ipAddress = getIPAddress(hostAddress);
                        client->hostName = getHostFromIp(client->ipAddress);
                        client->port = strdup(recvPacket->message);
                        addNode(&clientList, client);
                        printf("Registered client: %s %s/%s\n", client->hostName,
                               client->ipAddress, client->port);
                        free(recvPacket);

                        //send ack to client #needtomodify
                        char *message = "";
                        struct packet *pckt = packetBuilder(ack, NULL, strlen(message), message);
                        char *packetString = packetDecoder(pckt);
                        //printf("ACK Packet: %s\n",packetString);
                        int bytes_sent = send(clientsockfd, packetString, strlen(packetString), 0);

                        //Broadcast the client list to all the registered clients
                        //build the broadcast message
                        message = "";
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
                        broadcastToAllClients(clientList, message);
                        //printList(clientList);
                    }
                }
                else // handle data from clients
                {

                }
            }
        }
    }

}
