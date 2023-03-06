#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

pid_t fork_pids[5] = {0, 0, 0, 0, 0};


// Error function used for reporting issues
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in *address, int portNumber) {

    // Clear out the address struct
    memset((char *)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

void handle_connection(int connection_socket);

int await_next_connection(int listen_socket) {
    int connection_socket;
    struct sockaddr_in client_address;
    socklen_t size_of_client_info = sizeof(client_address);

    // Accept a connection, blocking if one is not available until one connects
    connection_socket = accept(listen_socket, (struct sockaddr *)&client_address, &size_of_client_info);
    if (connection_socket < 0) {
        error("ERROR on accept");
    }

    printf("SERVER(parent): Connected to client running at host %d port %d\n", ntohs(client_address.sin_addr.s_addr), ntohs(client_address.sin_port));

    pid_t pid = fork();
    if (pid == -1) {
        perror("forking failed");
        exit(1);
    } else if (pid == 0) {
        // Child process
        printf("Child process started.\n");
        close(listen_socket);

        printf("SERVER(child): Connected to client running at host %d port %d\n", ntohs(client_address.sin_addr.s_addr), ntohs(client_address.sin_port));
        handle_connection(connection_socket);
        

        printf("Child process closed.\n");
        exit(0);
    }
    // Parent process
    return pid;
}

#define MAX_BUFFER_SIZE 256

void dialog(int connection_socket);

void handle_connection(int connection_socket) {
    char buffer[MAX_BUFFER_SIZE];

    dialog(connection_socket);

/*
    // Get the message from the client and display it
    memset(buffer, '\0', MAX_BUFFER_SIZE);
    int chars_read = recv(connection_socket, buffer, MAX_BUFFER_SIZE - 1, 0);
    if (chars_read < 0) {
        perror("ERROR reading from socket");
    }
    printf("SERVER(child): I received this from the client: \"%s\"\n", buffer);

    // Send a Success message back to the client and close the connection
    chars_read = send(connection_socket, "I am the server, and I got your message", 39, 0);
    if (chars_read < 0) {
        perror("ERROR writing to socket");
    }
*/

    // Close the connection socket for this client


    close(connection_socket);

}


/*

Communication schema:
(connection started)
server: enc_server
client: enc_client|{message_length} -- maybe do enc_client|{block_nums} -- where block_nums is number of 256 byte blocks
client: {message}
sever: enc_server|{message_length}

*/

char* await_receive(int connection_socket, char *buffer, int buffer_size) {
    // Allocate the buffer if it hasn't been allocated yet
    if (buffer == NULL) {
        buffer = malloc(buffer_size);
        // assert(buffer != NULL);
    }

    // Clear the buffer
    memset(buffer, '\0', buffer_size);

    // Get the message from the client
    int chars_read = recv(connection_socket, buffer, buffer_size - 1, 0);
    if (chars_read < 0) {
        printf("ERROR reading from socket");
        exit(1);
    }

    // Display the message
    printf("SERVER(child) <-: \"%s\"\n", buffer);
    printf("SERVER(child) <-: \"%c\"\n", buffer[0]);

    // Make sure the buffer is null terminated
    // assert(chars_read == strlen(buffer));
    

    return buffer;
}

void dialog(int connection_socket) {
    int header_max_size = 256;
    char* header_buffer = await_receive(connection_socket, NULL, header_max_size);

    // // Parse the header
    
    int bar_idx = strcspn(header_buffer, "|");
    int header_size = atoi(header_buffer + bar_idx + 1);
    printf("Header size: %d\n", header_size);
    // printf("Header size: %d\n", header_size);

}

int main(int argc, char *argv[]) {
    int connectionSocket, charsRead;
    char buffer[256];
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1) {
        int pid = await_next_connection(listenSocket);
        for (int i = 0; i <= 5; i++) {
            if (i == 5) {
                printf("Too many connections, pool is full. Need to clean up. \n");
                exit(1);
            }
            if (fork_pids[i] == 0) {
                fork_pids[i] = pid;
                break;
            }
        }
    
        /*
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0) {
            error("ERROR on accept");
        }

        printf("SERVER: Connected to client running at host %d port %d\n", ntohs(clientAddress.sin_addr.s_addr), ntohs(clientAddress.sin_port));

        // Get the message from the client and display it
        memset(buffer, '\0', 256);
        // Read the client's message from the socket
        charsRead = recv(connectionSocket, buffer, 255, 0);
        if (charsRead < 0) {
            error("ERROR reading from socket");
        }
        printf("SERVER: I received this from the client: \"%s\"\n", buffer);
        sleep(2);
        // Send a Success message back to the client
        charsRead = send(connectionSocket, "I am the server, and I got your message", 39, 0);
        if (charsRead < 0) {
            error("ERROR writing to socket");
        }
        // Close the connection socket for this client
        close(connectionSocket);
    */
    }


    // Close the listening socket
    close(listenSocket);
    return 0;
}