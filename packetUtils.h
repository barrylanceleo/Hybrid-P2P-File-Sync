//
// Created by barry on 10/10/15.
//

#ifndef PEER_SYNC_PACKETUTILS_H
#define PEER_SYNC_PACKETUTILS_H

#define PACKET_SIZE 5000
enum messageTypes {
    ok,
    error,
    message,
    registerHost,
    hostList,
    connectHost,
    terminate,
    quit,
    get,
    put,
    syncFiles
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

int printPacket(struct packet *packet);

int deletePacketAndMessage(struct packet *pckt);

#endif //PEER_SYNC_PACKETUTILS_H
