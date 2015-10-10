//
// Created by barry on 10/10/15.
//

#ifndef PEER_SYNC_CLIENT_H
#define PEER_SYNC_CLIENT_H

int runClient(char *port);

int connectToHost(char *hostName, char *port);

int registerToServer(char *hostName, char *port);

int connectToClient(char *hostName, char *port);

#endif //PEER_SYNC_CLIENT_H
