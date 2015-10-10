#include "main.h"
#include "server.h"
#include "client.h"
#include "stringUtils.h"


int main(int argc, char *argv[]) {

    char *port = argv[2];

    if (argc != 3) {
        printf("Run the program as follows: \n programName s/c portNumber \n s -> server, c -> client\n");
        return -1;
    }

    if (stringEquals(argv[1], "s") || stringEquals(argv[1], "server")) {
        printf("Running as server..\n");
        runServer(port);
    }
    else if (stringEquals(argv[1], "c") || stringEquals(argv[1], "client")) {
        printf("Running as client..\n");
        runClient(port);
    }
    else {
        printf("Can't run programs with the given arguments.");
        printf("Run the program as follows: \n programName s/c portNumber \n s -> server, c -> client");
    }

    return 0;
}