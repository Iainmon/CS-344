#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>

int dialog_debug = 1;
char* _node_name = NULL;

void setup_dialog(char* name, int _dialog_debug) {
    _node_name = name;
    dialog_debug = _dialog_debug;
}

void flush_socket_recv(int connection_socket) {
    char b[] = "a";
    recv(connection_socket, b, 1, 0);
}

void flush_socket_send(int connection_socket) {
    char b[] = "a";
    send(connection_socket, b, 1, 0);
}


void await_send(int connection_socket, char* message) {
    int message_length = strlen(message);
    int chars_written = send(connection_socket, message, message_length, 0);
    if (chars_written < 0) {
        printf("CLIENT: ERROR writing message to socket");
    } else if (dialog_debug) {
        printf("wrote: \"%s\"\n", message);
    }
    if (chars_written < message_length) {
        printf("CLIENT: WARNING: Not all message data written to socket!\n");
    }

    flush_socket_recv(connection_socket);
}

void await_send_message(int connection_socket, char* message) {

    char header[256];
    memset(header, '\0', sizeof(header));
    sprintf(header, "%s|%d", _node_name, (int)strlen(message));

    await_send(connection_socket, header);
    await_send(connection_socket, message);

    // int chars_written = send(connection_socket, header, strlen(header),0);
    // if (chars_written < 0) {
    //     error("CLIENT: ERROR writing header to socket");
    // } else {
    //     printf("wrote: \"%s\"\n", header);
    // }
    // if (chars_written < strlen(header)) {
    //     printf("CLIENT: WARNING: Not all header data written to socket!\n");
    // }

    // flush_socket_recv(connection_socket);

    // int message_length = strlen(message);
    // chars_written = send(connection_socket, message, message_length, 0);
    // if (chars_written < 0) {
    //     error("CLIENT: ERROR writing message to socket");
    // } else {
    //     printf("wrote: \"%s\"\n", message);
    // }
    // if (chars_written < message_length) {
    //     printf("CLIENT: WARNING: Not all message data written to socket!\n");
    // }

    // flush_socket_recv(connection_socket);

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
        // exit(1);
    }

    // Display the message
    if (dialog_debug) { printf("SERVER(child) <- \"%s\"\n", buffer); }
    // printf("SERVER(child) <-: \"%c\"\n", buffer[0]);

    // Make sure the buffer is null terminated
    // assert(chars_read == strlen(buffer));
    

    return buffer;
}

char* await_receive_message(int connection_socket) {
    int header_max_size = 256;
    char* header_buffer = await_receive(connection_socket, NULL, header_max_size);

    // Parse the header
    int bar_idx = strcspn(header_buffer, "|");
    int header_size = atoi(header_buffer + bar_idx + 1);
    if (dialog_debug) { printf("[header]: %d\n", header_size); }

    flush_socket_send(connection_socket);

    char* message_buffer = await_receive(connection_socket, NULL, header_size + 1);
    if (dialog_debug) { printf("[message]: %s\n", message_buffer); }

    flush_socket_send(connection_socket);

    return message_buffer;
}

