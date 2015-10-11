//
// Created by barry on 10/10/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "packetUtils.h"
#include "stringUtils.h"

char *packetDecoder(struct packet *packet) {
    char *packetMessage;


    if (packet->header->messageType == put || packet->header->messageType == get ||
        packet->header->messageType == syncFile) {
        asprintf(&packetMessage, "%02d:%s:%d:%s", packet->header->messageType, packet->header->fileName,
                 packet->header->length, packet->message);
    }
    else if (packet->header->messageType == ack || packet->header->messageType == terminate) {
        asprintf(&packetMessage, "%02d:%d%s", packet->header->messageType, packet->header->length, "");
    }
    else {
        asprintf(&packetMessage, "%02d:%d:%s", packet->header->messageType, packet->header->length, packet->message);
    }
    return packetMessage;
}

struct packet *packetEncoder(char *packetString) {
    int outputsize;
    char **parts = splitString(packetString, ':', &outputsize);
    struct header *head = (struct header *) malloc(sizeof(struct header));
    struct packet *packet = (struct packet *) malloc(sizeof(struct packet));
    packet->header = head;
    packet->header->messageType = atoi(parts[0]);
    if (packet->header->messageType == put || packet->header->messageType == get ||
        packet->header->messageType == syncFile) {
        packet->header->fileName = parts[1];
        packet->header->length = atoi(parts[2]);
        packet->message = parts[3];
    }
    else if (packet->header->messageType == ack || packet->header->messageType == terminate) {
        packet->header->length = atoi(parts[1]);
        packet->message = "";
    }
    else {
        packet->header->length = atoi(parts[1]);
        packet->message = parts[2];
    }

    if (strlen(packet->message) > packet->header->length) {
        fprintf(stderr, "Length of message is longer than length specified in header");
    }
    return packet;
}

struct packet *packetBuilder(enum messageTypes messageType, char *fileName,
                             int length, char *message) {
    struct header *header = (struct header *) malloc(sizeof(struct header));
    struct packet *packet = (struct packet *) malloc(sizeof(struct packet));
    packet->header = header;
    header->messageType = messageType;
    header->length = length;
    header->fileName = fileName;
    packet->message = message;

    return packet;

}

int printPacket(struct packet *packet) {
    if (packet->header->messageType == put || packet->header->messageType == get ||
        packet->header->messageType == syncFile) {
        printf("Message Type: %d\nFile Name: %s\nMessage Length: %d\nMessage: %s\n", packet->header->messageType,
               packet->header->fileName,
               packet->header->length, packet->message);
    }
    else if (packet->header->messageType == ack || packet->header->messageType == terminate) {
        printf("Message Type: %d\nMessage Length: %d\nMessage: %s\n", packet->header->messageType,
               packet->header->length, "");
    }
    else {
        printf("Message Type: %d\nMessage Length: %d\nMessage: %s\n", packet->header->messageType,
               packet->header->length, packet->message);
    }
}

