#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


// #define BUFFER_SIZE 1024
// char buffer_1[BUFFER_SIZE];

#define MAX_LINE_SIZE 1024

#define MAX_BUFFER_ENTRIES 1024

struct thread_buffer {
    char* buffers[MAX_BUFFER_ENTRIES];
    int buffer_count;
    int producer_idx;
    int consumer_idx;
    pthread_mutex_t mutex;
    pthread_cond_t full;
};

struct thread_buffer* init_thread_buffer() {
    struct thread_buffer* tb = malloc(sizeof(struct thread_buffer));

    for (int i = 0; i < MAX_BUFFER_ENTRIES; i++) {
        tb->buffers[i] = NULL;
    }

    tb->buffer_count = 0;
    tb->producer_idx = 0;
    tb->consumer_idx = 0;

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t full = PTHREAD_COND_INITIALIZER;
    tb->mutex = mutex;
    tb->full = full;

    return tb;
}

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

    while (tb->buffer_count == 0) {
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&tb->full, &tb->mutex);
    }
    
    char* item = tb->buffers[tb->consumer_idx];                     // Get the item from the buffer
    char* item_copy = copy_str(item);                               // Copy the item

    tb->consumer_idx = (tb->consumer_idx + 1) % MAX_BUFFER_ENTRIES; // Increment the index from which the item will be picked up
    tb->buffer_count--;                                             // Decrement the buffer count

    pthread_mutex_unlock(&tb->mutex);                               // Unlock the mutex

    return item_copy;
}



struct thread_buffer* tb_1;


void* input_thread() {
    char* line_buff = malloc(sizeof(char) * MAX_LINE_SIZE);
    for (int i = 0; i < 10; i++) {
        memset(line_buff, '\0', MAX_LINE_SIZE);

        printf("[enter input]: ");
        fgets(line_buff, MAX_LINE_SIZE - 1, stdin);
        line_buff[strlen(line_buff) - 1] = '\0';

        printf("[input]: %s \n", line_buff);

        buffer_put(tb_1, line_buff);
    }
    return NULL;
}

void* output_thread() {
    for (int i = 0; i < 10; i++) {
        char* item = buffer_get(tb_1);
        printf("\n[output]: %s \n", item);
    }
    return NULL;
}


int main(int argv, char** argc) {

    pthread_t input_t;
    pthread_t output_t;

    tb_1 = init_thread_buffer();

    pthread_create(&input_t, NULL, input_thread, NULL);
    pthread_create(&output_t, NULL, output_thread, NULL);

    pthread_join(input_t, NULL);
    pthread_join(output_t, NULL);

    return 0;
}