//
// Created by barry on 10/9/15.
//

#ifndef PEER_SYNC_SERVER_H
#define PEER_SYNC_SERVER_H

int runServer(char *port);

int broadcastToAllClients(struct list *clientList, char *msg);

#endif //PEER_SYNC_SERVER_H
