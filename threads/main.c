#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


// #define BUFFER_SIZE 1024
// char buffer_1[BUFFER_SIZE];

#define OUTPUT_BUFFER_SIZE 80

#define MAX_LINE_SIZE 1024

#define MAX_BUFFER_ENTRIES 1024

struct thread_buffer {
    char* buffers[MAX_BUFFER_ENTRIES];
    int buffer_count;
    int producer_idx;
    int consumer_idx;
    pthread_mutex_t mutex;
    pthread_cond_t full;
    int finished;
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

    tb->finished = 0;

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

    while (tb->buffer_count == 0 && tb->finished == 0) {
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&tb->full, &tb->mutex);
    }
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



struct thread_buffer* tb_1;
struct thread_buffer* tb_2;
struct thread_buffer* tb_3;

int stop_processing = 0;

int producer_finished(struct thread_buffer* tb) {
    if (stop_processing != 0 && tb->buffer_count == 0) {
        return 1;
    }
    return 0;
}

void finish_thread_buffer(struct thread_buffer* tb) {
    pthread_mutex_lock(&tb->mutex);
    tb->finished = 1;
    pthread_cond_signal(&tb->full);
    pthread_mutex_unlock(&tb->mutex);
}


void* input_thread() {
    char* line_buff = malloc(sizeof(char) * MAX_LINE_SIZE);
    while (stop_processing == 0) {
        memset(line_buff, '\0', MAX_LINE_SIZE);

        // printf("[enter input]: ");
        fgets(line_buff, MAX_LINE_SIZE - 1, stdin);
        if (strcmp(line_buff, "STOP\n") == 0) {
            stop_processing = 1;
            finish_thread_buffer(tb_1);
            // printf("STOPPING PROCESSING\n");
            break;
        }

        buffer_put(tb_1, line_buff);
        // sleep(1);
        // printf("[input]: %s \n", line_buff);
    }
    return NULL;
}

void* line_separator_thread() {

    while (1) {
        // if (producer_finished(tb_1) == 1) { break; }

        char* line = buffer_get(tb_1);
        if (line == NULL) { 
            finish_thread_buffer(tb_2);
            break;
        }

        int length = strlen(line);

        for (int i = 0; i < length; i++) {
            char c = line[i];
            if (c == '\n') { line[i] = ' '; }
        }

        buffer_put(tb_2, line);
    }
    return NULL;
}

void* plus_sign_thread() {
    while (1) {

        char* line = buffer_get(tb_2);
        if (line == NULL) {
            finish_thread_buffer(tb_3);
            break;
        }

        int length = strlen(line);

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

        buffer_put(tb_3, new_line);
    }
    return NULL;
}



void* output_thread() {
    // for (int i = 0; i < 10; i++) {
    //     char* item = buffer_get(tb_2);
    //     printf("[output]: %s\n", item);
    // }
    // return NULL;

    char* output_buffer = malloc(sizeof(char) * OUTPUT_BUFFER_SIZE + 1);
    memset(output_buffer, '\0', OUTPUT_BUFFER_SIZE + 1);
    int next_output_buffer_idx = 0;

    while (1) {

        char* line = buffer_get(tb_3);
        if (line == NULL) { break; }

        int length = strlen(line);

        for (int i = 0; i < length; i++) {
            if (next_output_buffer_idx == OUTPUT_BUFFER_SIZE) {
                // Output buffer is full. Print it and reset the buffer
                // printf("[output]: %s\n", output_buffer);
                printf("%s\n", output_buffer);
                memset(output_buffer, '\0', OUTPUT_BUFFER_SIZE + 1);
                next_output_buffer_idx = 0;
            }
            output_buffer[next_output_buffer_idx] = line[i];
            next_output_buffer_idx++;
        }

        // printf("[output]: %s\n", line);

    }
    return NULL;
}


int main(int argv, char** argc) {

    pthread_t input_t;
    pthread_t line_separator_t;
    pthread_t plus_sign_t;
    pthread_t output_t;

    tb_1 = init_thread_buffer();
    tb_2 = init_thread_buffer();
    tb_3 = init_thread_buffer();

    pthread_create(&input_t, NULL, input_thread, NULL);
    pthread_create(&line_separator_t, NULL, line_separator_thread, NULL);
    pthread_create(&plus_sign_t, NULL, plus_sign_thread, NULL);
    pthread_create(&output_t, NULL, output_thread, NULL);

    pthread_join(input_t, NULL);
    pthread_join(line_separator_t, NULL);
    pthread_join(plus_sign_t, NULL);
    pthread_join(output_t, NULL);

    return 0;
}