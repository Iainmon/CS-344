#include <netdb.h> // gethostbyname()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // send(),recv()
#include <sys/types.h>  // ssize_t
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>
#include "dialog.c"

/**
 * Client code
 * 1. Create a socket and connect to the server specified in the command
 * arugments.
 * 2. Prompt the user for input and send that input as a message to the server.
 * 3. Print the message received from the server and exit the program.
 */

// Error function used for reporting issues
void error(const char *msg) {
    perror(msg);
    exit(0);
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in *address, int portNumber, char *hostname) {

    // Clear out the address struct
    memset((char *)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent *hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char *)&address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

// Returns the contents of the file as a string
char* getFileContents(char* filename);
int _main(int argc, char* argv[],char* message,char* key);
int main(int argc, char *argv[]) {

    char* messageFileName = argv[1];
    char* message = getFileContents(messageFileName);

    char* keyFileName = argv[2];
    char* key = getFileContents(keyFileName);
    key[strcspn(key,"\r\n")] = '\0';

    while (1) {
        _main(argc, argv,message,key);
        sleep(5);
    }
    return 0;
}



int _main(int argc, char* argv[],char* message,char* key) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    char buffer[256];
    // Check usage & args
    if (argc < 4) {
        fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
        exit(0);
    }
    
    char* messageFileName = argv[1];
    char* keyFileName = argv[2];
    int port = atoi(argv[3]);
    char hostname[] = "localhost";

    // char* message = getFileContents(messageFileName);
    // char* key = getFileContents(keyFileName);

    // printf("message: %s\n", message);
    // printf("key: %s\n", key);

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
    }


    // Set up the server address struct
    setupAddressStruct(&serverAddress, port, hostname);

    // Connect to server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("CLIENT: ERROR connecting");
    }

    // int yes = 1;
    // int result = setsockopt(socketFD,
    //                         IPPROTO_TCP,
    //                         TCP_NODELAY,
    //                         (char *) &yes, 
    //                         sizeof(int));    // 1 - on, 0 - off
    // if (result < 0) {
    //     error("CLIENT: ERROR setting socket options");
    // }


    // // Get input message from user
    // printf("CLIENT: Enter text to send to the server, and then hit enter: ");
    // // Clear out the buffer array
    // memset(buffer, '\0', sizeof(buffer));
    // // Get input from the user, trunc to buffer - 1 chars, leaving \0
    // fgets(buffer, sizeof(buffer) - 1, stdin);
    // // Remove the trailing \n that fgets adds
    // buffer[strcspn(buffer, "\n")] = '\0';
    // // message[strspn(message, "\n")] = '\0';

    // while(1) {
    // Send message to server
    // Write to the server
    // charsWritten = send(socketFD, buffer, strlen(buffer), 0);
    
    await_send_message(socketFD, message);
    await_send_message(socketFD, key);
    char* ciphertext = await_receive_message(socketFD);
    printf("ciphertext: %s\n", ciphertext);

    /*
    char header[256];
    memset(header, '\0', sizeof(header));
    sprintf(header, "enc_client|%d", strlen(message));

    charsWritten = send(socketFD, header, strlen(header),0);
    if (charsWritten < 0) {
        error("CLIENT: ERROR writing to socket");
    } else {
        printf("wrote: \"%s\"\n", header);
    }
    if (charsWritten < strlen(header)) {
        printf("CLIENT: WARNING: Not all data written to socket!\n");
    }
    */
    // write(socketFD, header, strlen(header));
    // // fflush(socketFD);
    // write(socketFD, message, strlen(message));


    // sleep(1);
    // fflush(socketFD);
    // flush_socket_recv(socketFD);
    /*
    charsWritten = send(socketFD, message, strlen(message), 0);
    if (charsWritten < 0) {
        error("CLIENT: ERROR writing to socket");
    } else {
        printf("wrote: \"%s\"\n", message);
    }
    if (charsWritten < strlen(message)) {
        printf("CLIENT: WARNING: Not all data written to socket!\n");
    }

    */
    // }
    // if (charsWritten < strlen(buffer)) {
    //     printf("CLIENT: WARNING: Not all data written to socket!\n");
    // }

    // Get return message from server
    // Clear out the buffer again for reuse
    /*
        memset(buffer, '\0', sizeof(buffer));
    // Read data from the socket, leaving \0 at end
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket");
    }
    printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
    */


    // Close the socket
    close(socketFD);
}


char* getFileContents(char* filename) {

    // Open the file
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "Error opening message file '%s'", filename);
        exit(1);
    }

    // Get the file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Allocate memory for the file contents
    char* string = malloc(fsize + 1);
    if (string == NULL) {
        fprintf(stderr, "Error allocating memory for file contents");
        exit(1);
    }

    // Read the file contents into the string
    fread(string, fsize, 1, f);
    fclose(f);
    
    // Add a null terminator
    string[fsize] = '\0';

    return string;

}