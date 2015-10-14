//
// Created by barry on 10/4/15.
//

#ifndef PEER_SYNC_LIST_H
#define PEER_SYNC_LIST_H

#include <stdbool.h>
#include <stdio.h>


struct list {
    void *value;
    FILE *filePointer; // this is to indicate if the connection is receiving a file, if NULL it is not writing to any file.
    struct list *next;
};

int addNode(struct list **head, void *value);

int printList(struct list *head);

bool isHostPresent(struct list *hostList, char *ipAddress, char *host);

struct list *removeNodeById(struct list *hostList, int id);

struct host *getHostByID(struct list *hostList, int id);

struct list *getNodeByID(struct list *hostList, int id);

int getIDForFD(struct list *hostList, int fd);


#endif //PEER_SYNC_LIST_H
