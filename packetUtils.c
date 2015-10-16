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

    if (packet->header->fileName == NULL) {
        packet->header->fileName = "";
    }
    if (packet->message == NULL) {
        packet->message = "";
    }

    asprintf(&packetMessage, "%02d^%d^%s^%s", packet->header->messageType, packet->header->length,
             packet->header->fileName, packet->message);

    //printf("PacketString: %s\n", packetMessage);

    return packetMessage;
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

    printf("Message Type: %d\nFile Name: %s\nMessage Length: %d\nMessage: %s\n", packet->header->messageType,
           packet->header->fileName, packet->header->length, packet->message);
}

int deletePacketAndMessage(struct packet *pckt)
{
    free(pckt->message);
    pckt->message = NULL;
    free(pckt->header);
    pckt->header = NULL;
    free(pckt);
    pckt = NULL;

    return 1;
}

int deletePacket(struct packet *pckt)
{
    free(pckt->header);
    free(pckt);
}