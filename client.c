//
// Created by barry on 9/29/15.
//
#include "list.h"
#include "socketUtils.h"
#include "client.h"
#include "packetUtils.h"
#include "main.h"
#include "server.h"
#include <errno.h>
#include <stdio.h>
#include <dirent.h>

fd_set masterFDList, tempFDList; //Master file descriptor list to add all sockets and stdin
int fdmax; //to hold the max file descriptor value
struct host *masterServer; //details about the master server
int connectionIdGenerator; //index for connections

int runClient(char *port) {
    char *name = "Client";

    //initialize masterServer, peerList and hostlist
    peerList = NULL;
    connectionList = NULL;
    connectionIdGenerator = 2; //1 is received for the server
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
                        client->id = connectionIdGenerator++;
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

                    }

                }
                else if (masterServer != NULL && fd == masterServer->sockfd)// handle data from server
                {

                    struct packet *recvPacket = readPacket(fd);
                    if (recvPacket == NULL) {
                        printf("Master Server has terminated unexpectedly.\n");
                        int connectionId = getIDForFD(connectionList, fd);
                        terminateConnection(connectionId);
                        masterServer = NULL;
                        peerList = NULL;
                        continue;
                    }

//                    printf("Received packet: ");
//                    printPacket(recvPacket);
//                    printf("\n");

                    //received terminate from server
                    if (recvPacket->header->messageType == terminate) {
                        printf("Received TERMINATE from server.\n");
                        int id = getIDForFD(connectionList, fd);
                        terminateClient(id);
                        peerList = NULL;
                        masterServer = NULL;
                        continue;
                    }

                    if (recvPacket->header->messageType == hostList) {
                        //split the hostlist
                        int length = 0;
                        char **hostinfo = splitString(recvPacket->message, ' ', &length);
                        free(peerList);
                        peerList = NULL; //destroy the old peerList
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
                        }
                        printf("New peerList received from server:\n");
                        printPeerList(peerList);
                        continue;
                    }

                    if (recvPacket->header->messageType == syncFiles) {
                        printf("Received a sync initiate request from Server.\n");
                        syncHostnameFiles();
                        continue;
                    }

                }
                else {

                    //handle data from the peers
                    struct packet *recvPacket = readPacket(fd);
                    if (recvPacket == NULL) {
                        //one of the clients terminated unexpectedly
                        int id = getIDForFD(connectionList, fd);
                        struct host *host = getHostByID(connectionList, id);
                        printf("Peer: %s/%s Sock FD:%d terminated unexpectedly. Removing it from the list.\n",
                               host->ipAddress, host->port, host->sockfd);
                        terminateConnection(id);
                        continue;
                    }
//                    printf("Received packet: ");
//                    printPacket(recvPacket);
//                    printf("\n");

                    //received terminate
                    if (recvPacket->header->messageType == terminate) {
                        printf("Received TERMINATE\n");
                        int id = getIDForFD(connectionList, fd);
                        terminateConnection(id);
                        continue;
                    }//handle error message
                    else if (recvPacket->header->messageType == error) {
                        printf("Received error message: %s\n", recvPacket->message);
                        continue;
                    }//handle get request
                    else if (recvPacket->header->messageType == get) {
                        int connectionId = getIDForFD(connectionList, fd);
                        struct host *destination = getHostByID(connectionList, connectionId);
                        if (destination == NULL) {
                            fprintf(stderr, "Coudn't find the connection id.\n");
                            continue;
                        }
                        printf("Received a get request for file %s from client: %s/%s.\n",
                               recvPacket->header->fileName, destination->ipAddress, destination->port);

                        sendFile(connectionId, recvPacket->header->fileName);
                    }//hand a put packet
                    else if (recvPacket->header->messageType == put) {
                        int connectionId = getIDForFD(connectionList, fd);
                        receiveFileAsynchronously(connectionId, recvPacket);
                        continue;
                    }//handle a ok packet
                    else if (recvPacket->header->messageType == ok) {
                        int connectionId = getIDForFD(connectionList, fd);
                        okPacketHandler(connectionId, recvPacket);
                        continue;
                    }

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
//    printf("Server IP Address: %s Port: %s\n",
//           getIPAddress(serverAddressInfo->ai_addr), getPort(serverAddressInfo->ai_addr));

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
        masterServer->id = 1;
        masterServer->hostName = hostName;
        masterServer->ipAddress = getIpfromHost("127.0.0.1"); // change to hostName
        masterServer->port = port;
        masterServer->sockfd = serversockfd;
        addNode(&connectionList, masterServer);

        //once registered send the listener port of the client to the server
        char *message = myListenerPort;
        struct packet *pckt = packetBuilder(registerHost, NULL, strlen(message), message);
        char *packetString = packetDecoder(pckt);
        //printf("Port Packet: %s\n", packetString);
        int bytes_sent = send(masterServer->sockfd, packetString, strlen(packetString), 0);
        if (bytes_sent != -1) {
            printf("Sent the listerner port %s to the server.\n", message);
        }
        free(pckt);
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
        client->id = connectionIdGenerator++;
        client->sockfd = clientSockfd;
        client->ipAddress = getIpfromHost(hostName);
        client->hostName = hostName;
        client->port = port;
        addNode(&connectionList, client);
        printf("Connected to client: %s on port: %s\n", client->hostName,
               client->port);
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

int terminateConnection(int connectionId) {
    struct host *host = getHostByID(connectionList, connectionId);
    if (host == NULL) {
        printf("Connection %d not found.\n", connectionId);
        return 0;
    }

    //send terminate message to the corresponding peer
    char *message = "";
    struct packet *pckt = packetBuilder(terminate, NULL, strlen(message), message);
    char *packetString = packetDecoder(pckt);
    //printf("TERMINATE Packet: %s\n", packetString);
    int bytes_sent = send(host->sockfd, packetString, strlen(packetString), 0);
    //printf("Sent Terminate: %d\n", bytes_sent);

    //close sock and remove it from fdlist and connection list
    close(host->sockfd);
    FD_CLR(host->sockfd, &masterFDList);
    connectionList = removeNodeById(connectionList, connectionId);
    if (connectionList == NULL) {
        printf("Got empty connection list\n");
    }
    //printf("Closed SockFd: %d\n", host->sockfd);
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
    //printf("TERMINATE Packet: %s\n", packetString);

    //send terminate message all connected peers
    int bytes_sent;
    struct list *current = connectionList;
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
    free(connectionList);
    exit(0);
}

int getFile(int connectionId, char *filename) {
    //get the destination host
    struct host *destination = getHostByID(connectionList, connectionId);
    if (destination == NULL) {
        printf("There is no connection with connection id: %d\n", connectionId);
        return -1;
    }

    //build a get request
    char *message = "";
    struct packet *pckt = packetBuilder(get, filename, strlen(message), message);
    char *packetString = packetDecoder(pckt);
    //printf("Get Request Packet: %s\n", packetString);

    //send the get request
    int bytes_sent = send(destination->sockfd, packetString, strlen(packetString), 0);
    if (bytes_sent != strlen(packetString)) {
        fprintf(stderr, "Get Request not fully sent.Bytes sent: %d, Packet Length: %d\n", bytes_sent,
                strlen(packetString));
        return -1;
    }
    return 0;
}

int putFile(int connectionId, char *filename) {

    //get the destination
    struct host *destination = getHostByID(connectionList, connectionId);
    if (destination == NULL) {
        printf("Count't find connection id to send file.\n");
        return -1;
    }

    //get the filename removing the directory names
    int filenamePartLength;
    char **filenameParts = splitString(strdup(filename), '/', &filenamePartLength);
    char *justFilename = filenameParts[filenamePartLength - 1];

    //open file
    FILE *fp = fopen(filename, "rb"); //rb is for opening binary files
    if (fp == NULL) {
        printf("Filename: %s, %s\n", filename, strerror(errno));
        return -2;
    }

    //Read data for first packet
    char buff[PACKET_SIZE];
    int data_read = 0;
    int bytes_read = fread(buff, 1, PACKET_SIZE, fp);
    if (bytes_read < PACKET_SIZE)
        buff[bytes_read] = 0;
    data_read += bytes_read;
    //printf("Bytes read %d \n", bytes_read);
    if (bytes_read <= 0 && ferror(fp)) {
        printf("Error reading file: %s\n", filename);
        fclose(fp);
        return -3;
    }
    struct packet *pckt = packetBuilder(put, justFilename, bytes_read, buff);
    char *packetString = packetDecoder(pckt);
//    printf("Packet String: %s\n", packetString);
//    printPacket(pckt);
    int bytes_sent = send(destination->sockfd, packetString, strlen(packetString), 0);

    //check if there is more data if so send them in packets
    while (true) {
        if (bytes_read < PACKET_SIZE) {
            if (feof(fp))
                //reached EOF no need to print anything just beak
            if (ferror(fp))
                printf("Error reading file: %s\n", filename);
            break;
        }
        bytes_read = fread(buff, 1, PACKET_SIZE, fp);
        if (bytes_read < PACKET_SIZE)
            buff[bytes_read] = 0;
        data_read += bytes_read;
        pckt = packetBuilder(put, justFilename, bytes_read, buff);
        packetString = packetDecoder(pckt);
//        printf("Packet String: %s\n", packetString);
//        printPacket(pckt);
        bytes_sent = send(destination->sockfd, packetString, strlen(packetString), 0);
    }
    printf("File: %s sent. Size %d bytes.\n", justFilename, data_read);

    //send a ok message to indicate completion.
    pckt = packetBuilder(ok, justFilename, 0, "");
    packetString = packetDecoder(pckt);
//    printf("OK Packet String: %s\n", packetString);
//    printPacket(pckt);
    bytes_sent = send(destination->sockfd, packetString, strlen(packetString), 0);
    return 0;
}

int sendFile(int connectionId, char *filename) // this is include the error message that need to be sent
{
    //get the filename removing the directory names
    int filenamePartLength;
    char **filenameParts = splitString(filename, '/', &filenamePartLength);
    char *justFilename = filenameParts[filenamePartLength - 1];

    //get the destination
    struct host *destination = getHostByID(connectionList, connectionId);
    if (destination == NULL) {
        printf("Count't find connection id to send file.\n");
        return -1;
    }

    //call putfile()
    int status = putFile(connectionId, filename);
    //incase of error sent error message to requestor
    if (status == -2) {
        //unable to open file send an error message to the requester.
        char *errorMessage;
        asprintf(&errorMessage, "%s", strerror(errno));
        struct packet *pckt = packetBuilder(error, justFilename, strlen(errorMessage), errorMessage);
        char *packetString = packetDecoder(pckt);
//        printf("Packet String: %s\n", packetString);
//        printPacket(pckt);
        int bytes_sent = send(destination->sockfd, packetString, strlen(packetString), 0);
        return -2;
    }
    else if (status == -3) {
        char *errorMessage;
        asprintf(&errorMessage, "Error reading file: %s", filename);
        struct packet *pckt = packetBuilder(error, filename, strlen(errorMessage), errorMessage);
        char *packetString = packetDecoder(pckt);
        //printf("Packet String: %s\n", packetString);
        //printPacket(pckt);
        int bytes_sent = send(destination->sockfd, packetString, strlen(packetString), 0);
        return -3;
    }
    return 0;
}

int receiveFileASynchronously(int connectionId, struct packet *recvPacket)
{
    struct host *source = getHostByID(connectionList, connectionId);
    printf("Receiving file: %s from client: %s/%s.\n",
           recvPacket->header->fileName, source->ipAddress, source->port);

    // Create file where data will be stored
    char *filename = recvPacket->header->fileName;
    char *tempfilename = stringConcat("/home/barry/Dropbox/Projects/PeerSync/received_files_2/", filename);
    FILE *fp = fopen(tempfilename, "wb"); //for testing it should be filename
    if (fp == NULL) {
        printf("Error opening file.\n");
        return -1;
    }

    //write the first packet received
    int written_bytes = fwrite(recvPacket->message, 1, strlen(recvPacket->message), fp);

    //keep receiving packets till you get a ok packet
    recvPacket = readPacket(source->sockfd);
    //printPacket(recvPacket);
    while (recvPacket->header->messageType != ok) {
        written_bytes += fwrite(recvPacket->message, 1, strlen(recvPacket->message), fp);
        recvPacket = readPacket(source->sockfd);
        //printPacket(recvPacket);
    }
    printf("File %s download complete. Size: %d bytes.\n", filename, written_bytes);
    fclose(fp);
    return 0;
}

int receiveFileAsynchronously(int connectionId, struct packet *recvPacket)
{
    struct list *connection = getNodeByID(connectionList, connectionId);
    struct host *source = (struct host *) connection->value;

    if (connection->filePointer == NULL) {
        printf("Receiving file: %s from client: %s/%s.\n",
               recvPacket->header->fileName, source->ipAddress, source->port);

        // Create file where data will be stored
        char *filename = recvPacket->header->fileName;
        char *tempfilename = stringConcat("/home/barry/Dropbox/Projects/PeerSync/received_files_2/", filename);
        FILE *fp = fopen(tempfilename, "wb"); //for testing it should be filename
        if (fp == NULL) {
            printf("Error opening file.\n");
            return -1;
        }

        //update the File pointer to the connection
        connection->filePointer = fp;

        //write the packet received
        int written_bytes = fwrite(recvPacket->message, 1, strlen(recvPacket->message), connection->filePointer);

    }
    else {
        //write the packet received
        int written_bytes = fwrite(recvPacket->message, 1, strlen(recvPacket->message), connection->filePointer);
    }
    return 0;
}

int okPacketHandler(int connectionId, struct packet *recvPacket)
{
    struct list *connection = getNodeByID(connectionList, connectionId);
    struct host *source = (struct host *) connection->value;

    //print completion of file download
    printf("File %s download complete.\n", recvPacket->header->fileName);
    //close the file pointer
    fclose(connection->filePointer);
    //make the file pointer in the connection to indicate that no file operation is in progress
    connection->filePointer = NULL;
    return 0;
}

int startSync() {
    //check if the client is registered to the server
    if (masterServer == NULL) {
        printf("Client is not registered to the server. Please register first to proceed.\n");
        return 0;
    }

    //build a sync packet
    struct packet *packet = packetBuilder(syncFiles, NULL, 0, NULL);
    char *packetString = packetDecoder(packet);
    //printf("Sync Packet: %s\n", packetString);

    //send sync message to the server for broadcast
    int bytes_sent = send(masterServer->sockfd, packetString, strlen(packetString), 0);
    printf("Sync request sent to server.\n");
    return 0;
}

int syncHostnameFiles()
{

    //open current directory and send all files to all the connected peers
    char *directory = "/home/barry/Dropbox/Projects/PeerSync/files_to_be_sent/";
    char *filename = stringConcat(myHostName, ".txt");

    //send file for all peers in connectionList
    struct list *listIterator = connectionList;
    struct host *destination;
    while (listIterator != NULL) {
        destination = (struct host *) listIterator->value;
        //if destination is the master server do not send
        if (masterServer != NULL && destination->sockfd == masterServer->sockfd) {
            listIterator = listIterator->next;
            continue;
        }
        filename = stringConcat(directory, filename);
        putFile(destination->id, filename);
        listIterator = listIterator->next;
    }
    //printf("Sent file: %s\n", filename);

    return 0;
}

int syncAll() {
    //open current directory and send all files to all the connected peers
    DIR *d;
    struct dirent *dir;
    char *directory = "/home/barry/Dropbox/Projects/PeerSync/files_to_be_sent/";
    char *filename = "";
    d = opendir(directory);
    if (d != NULL) {
        while ((dir = readdir(d)) != NULL) {
            //for each file
            if (dir->d_type == DT_REG) {
                //for all peers in connectionList
                struct list *listIterator = connectionList;
                struct host *destination;
                while (listIterator != NULL) {
                    destination = (struct host *) listIterator->value;
                    //if destination is the master server do not send
                    if (masterServer != NULL && destination->sockfd == masterServer->sockfd) {
                        listIterator = listIterator->next;
                        continue;
                    }
                    filename = stringConcat(directory, dir->d_name);
                    putFile(destination->id, filename);
                    listIterator = listIterator->next;
                }
                printf("Sent file: %s\n", filename);
            }
        }
        closedir(d);
    }
    else {
        printf("Unable to open directory.\n");
    }
    return 0;
}