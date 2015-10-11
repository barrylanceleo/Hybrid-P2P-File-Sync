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

int terminateConnection(int id); // terminate the given connection id

void quitClient();
#endif //PEER_SYNC_CLIENT_H
