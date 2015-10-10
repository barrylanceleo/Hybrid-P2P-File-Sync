//
// Created by barry on 9/29/15.
//
#include "list.h"
#include "socketUtils.h"
#include "client.h"

fd_set masterFDList, tempFDList; //Master file descriptor list to add all sockets and stdin
int fdmax; //to hold the max file descriptor value
int serverSockfd; //fd of the master server obtained after registering to it
struct list *peerList; //hostlist received from the server
struct list *connectionList; //list of hosts the client has a connection with

int runClient(char *port) {
    char *name = "Client";

    //initialize the peerList and hostlist
    peerList = NULL;
    connectionList = NULL;

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
                        client->sockfd = clientsockfd;
                        struct sockaddr *hostAddress = (struct sockaddr *) &clientaddr;
                        client->ipAddress = getIPAddress(hostAddress);
                        client->hostName = getHostFromIp(client->ipAddress);
                        client->port = getPort(hostAddress);
                        addNode(&connectionList, client);
                        printf("Accepted client: %s %s on port: %s\n", client->hostName,
                               client->ipAddress, client->port);

                        //send ack to client
                        char *msg = "ACK";
                        int bytes_sent = send(clientsockfd, msg, strlen(msg), 0);
                        printf("ACK sent %d", bytes_sent);
                    }

                }
                else if (fd == serverSockfd)// handle data from server
                {
                    char buffer[1000];
                    int bytes_received = recv(fd, buffer, 1000, 0);
                    buffer[bytes_received] = 0;
                    printf("%s\n", buffer);

                    //split the hostlist
                    int length = 0;
                    char **hostinfo = splitString(buffer, ' ', &length);
                    //peerList = NULL;
                    int i;
                    for (i = 0; i < length; i = i + 2) {
                        if (i + 1 >= length)
                            fprintf(stderr, "Disproportionate terms in hostList sent by server.\n");

                        if (stringEquals(myIpAddress, myIpAddress) && stringEquals(myListenerPort, port)) {
                            //this is so that the client doesn't add itself in the peerList
                            continue;
                        }

                        //if the host is not present in the list add it
                        if (!isHostPresent(peerList, hostinfo[i], hostinfo[i + 1])) {
                            struct host *peer = (struct host *) malloc(sizeof(struct host));
                            peer->sockfd = -1; // we do not have a connection with it yet
                            peer->ipAddress = hostinfo[i];
                            peer->hostName = getHostFromIp(peer->ipAddress);
                            peer->port = hostinfo[i + 1];
                            addNode(&peerList, peer);
                            printf("Added %s %s %s to peerList\n", peer->hostName, peer->ipAddress, peer->port);
                        }

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
    printf("Registering to Hostname: %s Port: %s\n", hostName, port);

    serverSockfd = connectToHost(hostName, port); //connect and get sockfd
    if (serverSockfd == -1) {
        fprintf(stderr, "Error Register to server\n");
    }
    else {
        //once registered send the listener port of the client to the server
        char *msg = myListenerPort;
        int bytes_sent = send(serverSockfd, msg, strlen(msg), 0);
        if (bytes_sent != -1) {
            printf("Sent the listerner port %s to the server.\n", msg);
        }
        //receive ack from the server #needtomodify
        char buffer[3];
        int bytes_received = recv(serverSockfd, buffer, 3, 0);
        buffer[bytes_received] = 0;
        printf("Received: %s\n", buffer);
    }
    return 0;
}

int connectToClient(char *hostName, char *port) {

    //Check if hostname/ip and port is registered to server
//    if(!isHostPresent(peerList, hostName, port))
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