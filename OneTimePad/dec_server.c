#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include "otp.c"
#include "dialog.c"



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

    // printf("SERVER(parent): Connected to client running at host %d port %d\n", ntohs(client_address.sin_addr.s_addr), ntohs(client_address.sin_port));

    pid_t pid = fork();
    if (pid == -1) {
        perror("forking failed");
        exit(1);
    } else if (pid == 0) {
        // Child process
        printf("[dec_server]: Child process started.\n");

        // Handle the connection
        handle_connection(connection_socket);
        
        printf("[dec_server]: Child process closed.\n");
        exit(0);
    }
    // Parent process
    return pid;
}

#define MAX_BUFFER_SIZE 256

void dialog(int connection_socket);

// Handle the connection, this just dispatches to the dialog function
void handle_connection(int connection_socket) {

    dialog(connection_socket);

    close(connection_socket);
}


void dialog(int connection_socket) {

    // Make sure client is dec_client

    char* client_hello = await_receive_message(connection_socket);
    if (strcmp(client_hello, "dec_client hello") != 0) {
        fprintf(stderr,"[dec_server]: Client did not say hello. Closing connection.\n");
        await_send_message(connection_socket, "dec_server hello");
        usleep(100000);
        close(connection_socket);
        exit(1);
    }
    printf("[dec_server]: Client said hello.\n");
    await_send_message(connection_socket, "dec_server hello");

    // retrieve the ciphertext and key
    char* ciphertext = await_receive_message(connection_socket);
    char* key = await_receive_message(connection_socket);

    // decrypt the message
    char* plaintext = decrypt_message(ciphertext, key);

    // output the paintext message
    printf("[dec_server]: plaintext length: %d, key length: %d, ciphertext length: %d\n", strlen(plaintext), strlen(key), strlen(ciphertext));

    // flush the socket
    usleep(FLUSH_DELAY + strlen(plaintext) * 2);

    // send the plaintext
    await_send_message(connection_socket, plaintext);
 
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

    // Name the node and start the dialog
    char node_name[] = "dec_server";
    int debug = 0;
    setup_dialog(node_name, debug);

    // Accept a connection, blocking if one is not available until one connects
    while (1) {
        // continuously fork and wait for connections
        int pid = await_next_connection(listenSocket);
    }


    // Close the listening socket
    close(listenSocket);
    return 0;
}