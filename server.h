//
// Created by barry on 10/9/15.
//

#ifndef PEER_SYNC_SERVER_H
#define PEER_SYNC_SERVER_H
struct list *clientList;


int runServer(char *port);
int broadcastToAllClients(struct list *clientList, char *msg);

int printClientList(struct list *head);

int terminateClient(int id);

void quitServer();
#endif //PEER_SYNC_SERVER_H
