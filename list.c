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

struct list *removeNodeById(struct list *hostList, int id) {
    struct list *current = hostList;
    struct host *currenthost = (struct host *) current->value;
    if (current == NULL) {
        fprintf(stdout, "List is empty\n");
        return NULL;
    }
    else if (current->next == NULL) {
        if (currenthost->id == id) {
            printf("First Node Host ID: %d Given ID: %d\n", currenthost->id, id);
            return current->next;
        }
    }
    else {
        if (currenthost->id == id) {
            printf("Host ID: %d Given ID: %d\n", currenthost->id, id);
            return current->next;
        }
        struct list *next = current->next;
        struct host *nexthost = (struct host *) next->value;
        do {
            printf("Host ID: %d Given ID: %d\n", nexthost->id, id);
            if (nexthost->id == id) {
                current->next = next->next;
                return hostList;
            }
        } while (next->next != NULL);
        fprintf(stdout, "Id: %d not found.\n", id);
        return hostList;
    }
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

int getIDdForFD(struct list *hostList, int fd) {
    struct list *current = hostList;
    if (current == NULL) {
        fprintf(stdout, "List is empty\n");
        return -1;
    }
    else {
        do {
            struct host *currenthost = (struct host *) current->value;
            printf("Current FD: %d Current ID:%d Given FD: %d\n", currenthost->sockfd, currenthost->id, fd);
            if (currenthost->sockfd == fd)
                return currenthost->id;
            current = current->next;
        } while (current != NULL);
        fprintf(stdout, "FD not found\n");
        return -1;
    }
}

struct host *getNodeByID(struct list *hostList, int id) {
    struct list *current = hostList;
    if (current == NULL) {
        fprintf(stdout, "List is empty\n");
        return NULL;
    }
    else {
        do {
            struct host *currenthost = (struct host *) current->value;
            printf("Host ID: %d Given ID: %d\n", currenthost->id, id);
            if (currenthost->id == id)
                return currenthost;
            current = current->next;
        } while (current != NULL);
        fprintf(stdout, "Id not found\n");
        return NULL;
    }
}