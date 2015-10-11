//
// Created by barry on 10/10/15.
//

#ifndef PEER_SYNC_PACKETUTILS_H
#define PEER_SYNC_PACKETUTILS_H

enum messageTypes {
    ack,
    registerHost,
    hostList,
    connectHost,
    terminate,
    quit,
    get,
    put,
    syncFile
};
struct header {
    enum messageTypes messageType;
    int length;
    char *fileName; //contains the filename if its for a file transfer

};
struct packet {
    struct header *header;
    char *message;
};

struct packet *packetBuilder(enum messageTypes messageType, char *fileName,
                             int length, char *message);

char *packetDecoder(struct packet *packet);

struct packet *packetEncoder(char *packetString);

int printPacket(struct packet *packet);

#endif //PEER_SYNC_PACKETUTILS_H
