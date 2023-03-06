#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#include "otp.c"
#include <string.h>


// This program creates a key file of specified length.

int main(int argc, char *argv[]) {
    // Check for correct number of arguments
    if (argc != 2) {
        fprintf(stderr,"Please enter a key length as the only argument.\n");
        exit(1);
    }


    // Convert argument to integer
    int length;
    length = atoi(argv[1]);

    // Allocate memory for string
    char *nums = malloc(length + 1);
    memset(nums, '\0', length + 1);

    // Seed random number generator
    srand(time(NULL));

    // Generate random characters
    int i = 0;
    while (i < length) {
        // Generate random number between 0 and 26
        int num = rand() % 27;
        // Convert to ASCII character
        char c = num + 64;
        // If the number is 0, convert to space
        if (num == 0) {
            c = ' ';
        }
        // Add character to string
        nums[i] = c;
        i++;
    }
    printf("%s\n", nums);

    // char* key = nums;
    // char* message = "HELLO WORLD";

    // char* encrypted = encrypt(message, key);
    // char* decrypted = decrypt(encrypted, key);

    // printf("msg: %s\n", message);
    // printf("enc: %s\n", encrypted);
    // printf("dec: %s\n", decrypted);

    return 0;
}

