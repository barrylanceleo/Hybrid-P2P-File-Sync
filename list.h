//
// Created by barry on 10/4/15.
//

#ifndef PEER_SYNC_LIST_H
#define PEER_SYNC_LIST_H

#include <stdbool.h>

struct list {
    void *value;
    struct list *next;
};

int addNode(struct list **head, void *value);

int printList(struct list *head);

bool isHostPresent(struct list *hostList, char *ipAddress, char *host);


#endif //PEER_SYNC_LIST_H