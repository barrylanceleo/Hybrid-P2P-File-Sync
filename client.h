//
// Created by barry on 10/10/15.
//

#ifndef PEER_SYNC_CLIENT_H
#define PEER_SYNC_CLIENT_H
struct list *peerList; //hostlist broadcasted by the server
struct list *connectionList; //list of hosts the client has a connection with
int runClient(char *port);

int connectToHost(char *hostName, char *port);

int registerToServer(char *hostName, char *port);

int connectToClient(char *hostName, char *port);

int printConnectionList(struct list *head);

int printPeerList(struct list *head);

int terminateConnection(int connectionId); // terminate the given connection id

void quitClient();

int getFile(int connectionId, char *filename);

int sendFile(int connectionId, char *filename);

int putFile(int connectionId, char *filename);

int startSync();

#endif //PEER_SYNC_CLIENT_H
