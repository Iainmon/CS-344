#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>


/*

Communication schema:
(connection started)
server: enc_server
client: enc_client|{message_length} -- maybe do enc_client|{block_nums} -- where block_nums is number of 256 byte blocks
client: {message}
sever: enc_server|{message_length}

*/

#define FLUSH_DELAY 500000

int dialog_debug = 1;
char* _node_name = NULL;

int last_sent = 0;

void setup_dialog(char* name, int _dialog_debug) {
    _node_name = name;
    dialog_debug = _dialog_debug;
}


void constant_flush() {
    usleep(FLUSH_DELAY);
}

void await_send(int connection_socket, char* message) {

    // Send the message
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
    
}

void await_send_message(int connection_socket, char* message) {

    // construct message header
    char header[256];
    memset(header, '\0', sizeof(header));
    sprintf(header, "%s|%d", _node_name, (int)strlen(message));

    // Send the header
    await_send(connection_socket, header);
    constant_flush();

    // Send the message
    await_send(connection_socket, message);
    constant_flush();

    // flush the socket depending on the size of the message
    usleep(FLUSH_DELAY + strlen(message) * 2);
}





char* await_receive(int connection_socket, char *buffer, int buffer_size) {
    // Allocate the buffer if it hasn't been allocated yet
    if (buffer == NULL) {
        buffer = malloc(buffer_size);
    }

    // Clear the buffer
    memset(buffer, '\0', buffer_size);

    // Get the message from the client
    int chars_read = recv(connection_socket, buffer, buffer_size - 1, 0);
    if (chars_read < 0) {
        fprintf(stderr,"ERROR reading from socket\n");
        exit(1);
    }
    last_sent = 0;

    // Display the message
    if (dialog_debug) { printf("node <- \"%s\"\n", buffer); }

    return buffer;
}

char* await_receive_message(int connection_socket) {

    // Receive the header
    int header_max_size = 256;
    char* header_buffer = await_receive(connection_socket, NULL, header_max_size);

    // Parse the header
    int bar_idx = strcspn(header_buffer, "|");
    int header_size = atoi(header_buffer + bar_idx + 1);
    if (dialog_debug) { printf("[header]: %d\n", header_size); }

    // flush socket
    constant_flush();

    // Receive the message
    usleep(FLUSH_DELAY + header_size * 2);
    char* message_buffer = await_receive(connection_socket, NULL, header_size + 1);
    if (dialog_debug) { printf("[message]: %s\n", message_buffer); }

    // flush socket
    constant_flush();

    return message_buffer;
}

