#include "main.h"
#include "server.h"
#include "client.h"
#include "stringUtils.h"
#include "socketUtils.h"

void printCommands() {
    printf("1. HELP\n2. CREATOR\n3. DISPLAY\n4. REGISTER <server IP> <port no>\n"
                   "5. CONNECT <destination> <port no>\n6. LIST\n7. TERMINATE <connection id>\n"
                   "8. QUIT\n9. GET <connection id> <file>\n10. PUT <connection id> <file name>\n"
                   "11. SYNC\n");
}

int handleCommands(char *command, char *hostType) {
    //remove the last \n in command if present
    if (command[strlen(command) - 1] == '\n')
        command[strlen(command) - 1] = '\0';

    //split the command into parts
    int commandLength = 0;
    char **commands = splitString(command, ' ', &commandLength);

//    printf("Number of commands: %d\n", commandLength);
//    int i;
//    for(i=0;i<commandLength;i++)
//        printf("--%s--\n", commands[i]);
//
//    printf("command[0] --%s--\n", commands[0]);

    if (commandLength == 1 && stringEquals(commands[0], "HELP")) {
        printCommands();
    }
    else if (commandLength == 1 && stringEquals(commands[0], "CREATOR")) {
        printf("Name: Barry Lance Leo Wilfred\n"
                       "UBIT Name: barrylan\n"
                       "UB E-mail: barrylan@buffalo.edu\n");
    }
    else if (commandLength == 1 && stringEquals(commands[0], "DISPLAY")) {
        printf("Name: %s\nIPAddress:  %s\nPort: %s\n", myHostName, myIpAddress, myListenerPort);
    }
    else if (commandLength == 3 && stringEquals(commands[0], "REGISTER")) {
        if (stringEquals(hostType, "SERVER")) {
            printf("This is the master server. Only the clients can register to the server\n");
        }
        else {
            printf("Registering to Server: %s/%s \n", commands[1], commands[2]);
            registerToServer(commands[1], commands[2]);
        }
    }
    else if (commandLength == 3 && stringEquals(commands[0], "CONNECT")) {
        if (stringEquals(hostType, "SERVER")) {
            printf("This is the master server. Only the clients can connect to other clients\n");
        }
        else {
            printf("Connecting to client\n");
            connectToClient(commands[1], commands[2]);
        }
    }
    else if (commandLength == 1 && stringEquals(commands[0], "LIST")) {
        if (stringEquals(hostType, "SERVER")) {
            printClientList(clientList);
        }
        else {
            printConnectionList(connectionList);
        }
    }
    else if (commandLength == 2 && stringEquals(commands[0], "TERMINATE")) {
        if (stringEquals(hostType, "SERVER")) {
            printf("This is the master server. Only clients can terminate the connections of their peers\n");
        }
        else {
            terminateConnection(atoi(commands[1]));
        }
    }
    else if (commandLength == 1 && stringEquals(commands[0], "QUIT")) {
        if (stringEquals(hostType, "SERVER")) {
            quitServer();
        }
        else {
            printf("Quitting Client.\n");
            quitClient();
        }
    }
    else {
        printf("Unsupported Command. Type \"help\" for help\n");
    }
    return 0;
}

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