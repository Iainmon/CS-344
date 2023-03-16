#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


#define OUTPUT_BUFFER_SIZE 80

#define MAX_LINE_SIZE 1024

#define MAX_BUFFER_ENTRIES 1024

// Struct for each intermediate buffer
struct thread_buffer {
    char* buffers[MAX_BUFFER_ENTRIES];
    int buffer_count;
    int producer_idx;
    int consumer_idx;
    pthread_mutex_t mutex;
    pthread_cond_t full;
    int finished;
};

// Create a thread buffer
struct thread_buffer* init_thread_buffer() {
    struct thread_buffer* tb = malloc(sizeof(struct thread_buffer));

    // Initialize the buffers
    for (int i = 0; i < MAX_BUFFER_ENTRIES; i++) {
        tb->buffers[i] = NULL;
    }

    // Initialize the buffer count, producer index, and consumer index
    tb->buffer_count = 0;
    tb->producer_idx = 0;
    tb->consumer_idx = 0;

    // Initialize the mutex and condition variable
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t full = PTHREAD_COND_INITIALIZER;
    tb->mutex = mutex;
    tb->full = full;

    tb->finished = 0;

    return tb;
}

// Helper function for copying a string
char* copy_str(char* str) {
    if (str == NULL) { return NULL; }
    int length = strlen(str) + 1;
    char* copy = malloc(sizeof(char) * length);
    memset(copy, '\0', length);
    strcpy(copy, str);
    return copy;
}

void buffer_put(struct thread_buffer* tb, char* item) {
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&tb->mutex);

    char* item_copy = copy_str(item);                               // Copy the item
    tb->buffers[tb->producer_idx] = item_copy;                      // Put the item in the buffer
    tb->producer_idx = (tb->producer_idx + 1) % MAX_BUFFER_ENTRIES; // Increment the index where the next item will be put.
    tb->buffer_count++;                                             // Increment the buffer count

    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&tb->full);
    // Unlock the mutex
    pthread_mutex_unlock(&tb->mutex);
}

char* buffer_get(struct thread_buffer* tb) {
    pthread_mutex_lock(&tb->mutex);

    while (tb->buffer_count == 0 && tb->finished == 0) {
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&tb->full, &tb->mutex);
    }

    // Buffer is empty and the producer has finished
    if (tb->buffer_count == 0 && tb->finished == 1) {
        pthread_mutex_unlock(&tb->mutex);
        return NULL;
    }
    
    char* item = tb->buffers[tb->consumer_idx];                     // Get the item from the buffer
    char* item_copy = copy_str(item);                               // Copy the item

    tb->consumer_idx = (tb->consumer_idx + 1) % MAX_BUFFER_ENTRIES; // Increment the index from which the item will be picked up
    tb->buffer_count--;                                             // Decrement the buffer count

    pthread_mutex_unlock(&tb->mutex);                               // Unlock the mutex

    return item_copy;
}

// Function that tells the consumer that the producer has finished
void finish_thread_buffer(struct thread_buffer* tb) {
    pthread_mutex_lock(&tb->mutex);
    tb->finished = 1;
    pthread_cond_signal(&tb->full);
    pthread_mutex_unlock(&tb->mutex);
}


// Shared buffers
struct thread_buffer* tb_1;
struct thread_buffer* tb_2;
struct thread_buffer* tb_3;


void* input_thread() {

    // Flag to stop processing new input
    int stop_processing = 0;

    char* line_buff = malloc(sizeof(char) * MAX_LINE_SIZE);

    while (stop_processing == 0) {
        // Clear the line buffer
        memset(line_buff, '\0', MAX_LINE_SIZE);

        // Read a line from the input
        fgets(line_buff, MAX_LINE_SIZE - 1, stdin);

        // Check if the input is STOP
        if (strcmp(line_buff, "STOP\n") == 0) {
            stop_processing = 1;
            finish_thread_buffer(tb_1);
            break;
        }

        // Produce the line
        buffer_put(tb_1, line_buff);

    }
    return NULL;
}

void* line_separator_thread() {

    while (1) {

        // Check if the producer has finished
        char* line = buffer_get(tb_1);
        if (line == NULL) { 
            // The producer has finished
            finish_thread_buffer(tb_2);
            break;
        }

        int length = strlen(line);

        // Replace newlines with spaces
        for (int i = 0; i < length; i++) {
            char c = line[i];
            if (c == '\n') { line[i] = ' '; }
        }

        // Produce the line
        buffer_put(tb_2, line);
    }
    return NULL;
}

void* plus_sign_thread() {

    while (1) {

        // Check if the producer has finished
        char* line = buffer_get(tb_2);
        if (line == NULL) {
            // The producer has finished
            finish_thread_buffer(tb_3);
            break;
        }

        int length = strlen(line);

        // Replace two consecutive plus signs with a caret
        char* new_line = malloc(sizeof(char) * length + 1);
        memset(new_line, '\0', length + 1);

        int found = 0;
        int new_line_idx = 0;

        for (int i = 0; i < length; i++) {
            char c = line[i];
            if (c == '+') {
                found++;
            } else {
                found = 0;
            }
            if (found == 2) {
                new_line[new_line_idx - 1] = '^';
                found = 0;
            } else {
                new_line[new_line_idx] = c;
                new_line_idx++;
            }
        }

        // Produce the line
        buffer_put(tb_3, new_line);
    }
    return NULL;
}



void* output_thread() {

    // Allocate the output buffer
    char* output_buffer = malloc(sizeof(char) * OUTPUT_BUFFER_SIZE + 1);
    memset(output_buffer, '\0', OUTPUT_BUFFER_SIZE + 1);
    int next_output_buffer_idx = 0;

    while (1) {

        // Check if the producer has finished
        char* line = buffer_get(tb_3);
        if (line == NULL) { break; }

        int length = strlen(line);

        // Add the line to the output buffer
        for (int i = 0; i < length; i++) {
            if (next_output_buffer_idx == OUTPUT_BUFFER_SIZE) {
                // Output buffer is full. Print it and reset the buffer
                printf("%s\n", output_buffer);
                memset(output_buffer, '\0', OUTPUT_BUFFER_SIZE + 1);
                next_output_buffer_idx = 0;
            }
            // Add the character to the output buffer
            output_buffer[next_output_buffer_idx] = line[i];
            next_output_buffer_idx++;
        }

    }
    return NULL;
}


int main(int argv, char** argc) {

    // Allocate the threads
    pthread_t input_t;
    pthread_t line_separator_t;
    pthread_t plus_sign_t;
    pthread_t output_t;

    // Initialize the thread buffers
    tb_1 = init_thread_buffer();
    tb_2 = init_thread_buffer();
    tb_3 = init_thread_buffer();

    // Create the threads
    pthread_create(&input_t, NULL, input_thread, NULL);
    pthread_create(&line_separator_t, NULL, line_separator_thread, NULL);
    pthread_create(&plus_sign_t, NULL, plus_sign_thread, NULL);
    pthread_create(&output_t, NULL, output_thread, NULL);

    // Wait for the threads to finish
    pthread_join(input_t, NULL);
    pthread_join(line_separator_t, NULL);
    pthread_join(plus_sign_t, NULL);
    pthread_join(output_t, NULL);

    return 0;
}