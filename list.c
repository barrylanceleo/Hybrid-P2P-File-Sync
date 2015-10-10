//
// Created by barry on 10/4/15.
//

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "socketUtils.h"

int addNode(struct list **head, void *value) {
    struct list *current = *head;
    if (value == NULL) {
        fprintf(stderr, "Value is NULL in addNode()\n");
        return -1;
    }
    struct list *node = malloc(sizeof(struct list));
    node->value = value;
    node->next = NULL;
    if (*head == NULL) {
        *head = node;
        (*head)->next = NULL;
        //printf("adding head\n");
    }
    else {
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = node;
        current->next->next = NULL;
        //printf("adding other nodes\n");
    }
    return 0;
}

int printList(struct list *head) {
    struct list *current = head;
    if (current == NULL) {
        fprintf(stdout, "List is empty\n");
        return 0;
    }
    else {
        do {
            struct host *currenthost = (struct host *) current->value;
            printf("FD: %d\n", currenthost->sockfd);
            current = current->next;
        } while (current != NULL);
    }
    return 0;
}

//checks if hostname/port or ipaddress/port is present in the list
bool isHostPresent(struct list *hostList, char *ipAddress, char *port) {
    char *hostName = getHostFromIp(ipAddress);
    struct list *current = hostList;
    if (current == NULL) {
        return false;
    }
    while (current != NULL) {
        struct host *currentHost = (struct host *) (current->value);
        if ((stringEquals(ipAddress, currentHost->ipAddress) || stringEquals(hostName, currentHost->hostName))
            && stringEquals(port, currentHost->port)) {
            return true;
        }
        current = current->next;
    }
    return false;
}