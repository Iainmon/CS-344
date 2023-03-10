
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// Movie struct that holds the data for a movie
struct movie_t {
    char* title;
    int year;
    char languages[5][20];
    float rating;
};

// Linked list of movies
struct movie_list_t {
    struct movie_t * movie;
    struct movie_list_t * next;
};

// Returns the tail of the list
struct movie_list_t * get_last(struct movie_list_t * ml) {
    struct movie_list_t * curr = ml;
    // If the list is empty, create a new node
    if (curr == NULL) {
        curr = malloc(sizeof(struct movie_list_t));
        assert(curr != NULL);
        curr->movie = NULL;
        curr->next = NULL;
        return curr;
    }
    // Otherwise, iterate to the end of the list
    while (curr->next != NULL) {
        curr = curr->next;
    }
    // Return the last node
    curr->next = NULL;
    return curr;
}

// Appends a movie to the end of the list
struct movie_list_t * append(struct movie_list_t * ml, struct movie_t m) {
    // Get the last node in the list
    struct movie_list_t * head = ml;
    struct movie_list_t * last = get_last(head);
    assert(last != NULL);
    struct movie_list_t * fresh = get_last(NULL); // Creates a blank list node
    assert(fresh != NULL);

    // Append the new node to the end of the list
    last->next = fresh;

    // If the list is empty, set the head to the new node
    if (last->movie == NULL) {
        // free(last);
        head = fresh;
    }

    // Copy the movie data into the new node
    struct movie_t * movie = malloc(sizeof(struct movie_t));
    assert(movie != NULL);
    *movie = m;
    movie->title = malloc(strlen(m.title) + 1);
    strcpy(movie->title, m.title);
    
    // Set the new node's movie pointer to the new movie
    fresh->movie = movie;
    return head;
}

// Helper function that splits a string by a delimiter and returns an array of tokens
char** tokenize(char* line, char* delim, int * length) {

    int num = 0;
    // Copy string so that strtok_r doesn't modify the original
    char* rest = NULL;
    char* token;
    char* line_ = calloc(strlen(line) + 1,sizeof(char));
    strcpy(line_,line);

    // Count the number of tokens
    token = strtok_r(line_, delim, &rest);
    while (token != NULL) {
        num++;
        token = strtok_r(NULL, delim, &rest);
    }

    // Allocate memory for the tokens
    char** tokens = malloc(sizeof(char) * (num + 1));

    // Copy the tokens into the array
    int i = 0;
    token = strtok(line, delim);
    while (i < num) {
        tokens[i] = token;
        token = strtok(NULL,delim);
        i++;
    }
    // Set the length pointer to the number of tokens
    *length = i;

    return tokens;
}

// Helper function that prints a movie (for debugging)
void print_movie(struct movie_t * movie) {
    printf(
        "movie { title = '%s', year = %d, rating = %.1f, languages = ",
        movie->title,
        movie->year,
        movie->rating);
    printf("[");
    for (int i = 0; i < 5; i++) { // Print the languages
        if (movie->languages[i][0] == 0) { continue; }
        printf("%s %s ", (i > 0 ? "," : ""), movie->languages[i]);
    }
    printf("] }\n");
}

// Helper function that prints a list of movies (for debugging)
void print_movie_list(struct movie_list_t * head) {
    int i = 0;
    while (head != NULL) {
        assert(head->movie != NULL);
        printf("[%d] ",i);
        print_movie(head->movie);
        i++;
        head = head->next;
    }
}

// Helper function that populates the languages array in a movie struct
void populate_languages(char * lang_str, struct movie_t * movie /* char (*languages)[5][20]*/) {
    int length = strlen(lang_str);
    int num = 0;
    int i = 0;
    int j = 0;
    // Iterate through the string and populate the languages array via a simple state machine
    while (i < length) {
        char c = lang_str[i];
        if (c == '[' || c == ']') { i++; continue; }
        if (c == ';') {
            num++;
            j = 0;
            i++;
            continue;
        }
        movie->languages[num][j] = c;
        j ++;
        i++;
    }
}


// Helper function that constructs a movie struct from a line of CSV data
struct movie_t construct_movie(char* line) {

    struct movie_t movie;

    // Initialize the languages array
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 20; j++) {
            movie.languages[i][j] = 0;
        }
    }

    int num_tokens;
    char** tokens = tokenize(line, ",", &num_tokens);

    // Populate the movie struct
    movie.title = tokens[0];
    movie.year = (int) atof(tokens[1]);
    populate_languages(tokens[2], &movie);
    movie.rating = atof(tokens[3]);

    // Free the tokens array
    // free(tokens);

    return movie;
}

// Helper function that returns the length of a linked list
int list_length(struct movie_list_t * head) {
    int i = 0;
    while (head != NULL) {
        i++;
        head = head->next;
    }
    return i;
}

// Routine that prints the titles of all movies released in a given year
void print_movies_by_year(struct movie_list_t * head, int year) {
    int printed_movies = 0;
    while (head != NULL) {
        // Make sure the movie pointer is not NULL
        assert(head->movie != NULL);
        // Print the movie title if the year matches
        if (head->movie->year == year) {
            printf("%s\n", head->movie->title);
            printed_movies++;
        }
        head = head->next;
    }
    // Print a message if no movies were found
    if (printed_movies == 0) {
        printf("No data about movies released in the year %d\n", year);
    }
}

// Helper function that returns the index of the highest rated movie in a given year
int highest_rated_in_year(struct movie_list_t * head, int year) {
    int highest_rated = 0;
    int i = 0;
    int highest_rated_idx = 0;
    // Iterate through the list and find the highest rated movie in the given year
    while (head != NULL) {
        assert(head->movie != NULL);
        if (head->movie->year == year) {
            if (head->movie->rating > highest_rated) {
                // If the movie's rating is higher than the current highest rated movie, update the highest rated movie
                highest_rated = head->movie->rating;
                highest_rated_idx = i;
            }
        }
        i++;
        head = head->next;
    }
    // Return the index of the highest rated movie
    return highest_rated_idx;
}

// Routine that prints the titles of the highest rated movie in each year
void print_movies_by_highest_rated_per_year(struct movie_list_t * head) {
    struct movie_list_t * current = head;
    int printed_movies = 0;
    int i = 0;
    // Iterate through the list and print the highest rated movie in each year
    while (current != NULL) {
        // Make sure the movie pointer is not NULL
        assert(current->movie != NULL);

        // Get the year of the current movie
        int year = current->movie->year;
        int highest_rated_idx = highest_rated_in_year(head, year);

        // Print the movie if it is the highest rated movie in the given year
        if (i == highest_rated_idx) {
            printf("%d %.1f %s\n", current->movie->year, current->movie->rating, current->movie->title);
            printed_movies++;
        }
        i++;
        current = current->next;
    }
    // Print a message if no movies were found
    if (printed_movies == 0) {
        printf("No movies present.\n");
    }
}

// Routine that prints the titles of all movies in a given language
void print_movies_in_language(struct movie_list_t * head, char* language) {
    // Iterate through the list
    int printed_movies = 0;
    while (head != NULL) {
        // Make sure the movie pointer is not NULL
        assert(head->movie != NULL);

        // Iterate through the languages array
        for (int i = 0; i < 5; i++) {
            // Print the movie if the language matches
            if (strcmp(head->movie->languages[i], language) == 0) {
                printf("%d %s\n", head->movie->year, head->movie->title);
                printed_movies++;
                break;
            }
        }
        head = head->next;
    }
    // Print a message if no movies were found
    if (printed_movies == 0) {
        printf("No movies in %s.\n", language);
    }
}

// Helper function that returns the size of a file
int get_file_size(char* file_name) {
    struct stat f;
    stat(file_name, &f);
    int size = f.st_size;
    return size;
}

void get_max_file_name(char * max_file_ptr) {

    // Get list of files in the directory
    FILE * cmd;
    char line_buffer[255] = {0};
    cmd = popen("/bin/ls -1 movies_*.csv", "r");
    if (cmd == NULL) {
        printf("Error: Failed to run command.\n");
        exit(1);
    }

    // Find the file with the largest size

    int max_size = 0;

    while (fgets(line_buffer, sizeof(line_buffer), cmd) != NULL) {
        line_buffer[strcspn(line_buffer, "\r\n")] = 0;
        int size = get_file_size(line_buffer);
        if (size > max_size) {
            max_size = size;
            strcpy(max_file_ptr, line_buffer);
        }
    }
}

void get_min_file_name(char * min_file_ptr) {

    // Get list of files in the directory
    FILE * cmd;
    char line_buffer[255] = {0};
    cmd = popen("/bin/ls -1 movies_*.csv", "r");
    if (cmd == NULL) {
        printf("Error: Failed to run command.\n");
        exit(1);
    }

    // Find the file with the smallest size
    int min_size = 0x7FFFFFFF;

    while (fgets(line_buffer, sizeof(line_buffer), cmd) != NULL) {
        line_buffer[strcspn(line_buffer, "\r\n")] = 0;
        int size = get_file_size(line_buffer);
        if (size < min_size) {
            min_size = size;
            strcpy(min_file_ptr, line_buffer);
        }
    }
}

struct movie_list_t * read_movies(char * file_name) {

    // Open the file
    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Error: Failed to open file %s\n", file_name);
        exit(1);
    }

    char* line = NULL;
    int line_length = 0;

    struct movie_list_t * head = NULL;

    // Read the file line by line
    while (getline(&line, &line_length, file) != -1) {
        line[strcspn(line, "\r\n")] = 0;              // Remove the newline character
        struct movie_t movie = construct_movie(line); // Construct a movie from the line
        head = append(head,movie);                    // Append the movie to the list
    }
    fclose(file); // Close the file
    
    // If the list is empty, print an error message and exit
    // This will happen if the column headers are present, but not the data
    if (head->next != NULL) {
        head = head->next;
    } else {
        printf("No movies present.\n");
        exit(1);
    }

    return head;
}

void write_years(struct movie_list_t * head, char * folder_name) {
    // Iterate through linked list and append the movie title to the file
    struct movie_list_t * curr;
    curr = head;
    char* cmd_buff = malloc(sizeof(char) * 1024);
    while (curr != NULL) {
        // Append the movie title to the file
        assert(curr->movie != NULL);
        sprintf(cmd_buff, "echo \"%s\" >> %s/%d.txt", curr->movie->title, folder_name, curr->movie->year);
        FILE * cmd = popen(cmd_buff, "r");
        if (cmd == NULL) {
            printf("Error: Failed to write to file :  %s/%d.txt\n", folder_name, curr->movie->year);
            exit(1);
        }
        pclose(cmd);

        curr = curr->next;
    }
}

// Function to create a folder with a random name
char * make_folder() {
    // Set the random seed
    srand(time(NULL));
    int min_num = 0;
    int max_num = 99999;
    // Generate a random number between 0 and 99999
    int random_number = rand() % (max_num + 1 - min_num) + min_num;

    // Create the folder name
    char * folder_name = malloc(sizeof(char) * 255);
    sprintf(folder_name, "moncrief.movies.%d", random_number);

    // Create the folder
    char cmd_buffer[255];
    sprintf(cmd_buffer, "mkdir %s", folder_name);
    FILE * cmd = popen(cmd_buffer, "r");
    if (cmd == NULL) {
        printf("Error: Failed to run command. | %s\n", folder_name);
        exit(1);
    }
    pclose(cmd);

    return folder_name;
}

// Function to set the permissions of the folder and files
void set_permissions(char * folder_name) {
    // Set the permissions of the files
    char cmd_buffer_1[255];
    sprintf(cmd_buffer_1, "chmod 640 %s/*.txt", folder_name);
    FILE * cmd = popen(cmd_buffer_1, "r");
    if (cmd == NULL) {
        printf("Error: Failed to run command. | %s\n", cmd_buffer_1);
        exit(1);
    }
    pclose(cmd);

    // Set the permissions of the folder
    char cmd_buffer_2[255];
    sprintf(cmd_buffer_2, "chmod 750 %s", folder_name);
    cmd = popen(cmd_buffer_2, "r");
    if (cmd == NULL) {
        printf("Error: Failed to run command. | %s\n", cmd_buffer_2);
        exit(1);
    }
    pclose(cmd);
}

void get_input_file_name(char * file_name) {

    // Display menu
    printf("\nWhich file you want to process?\n");
    printf("Enter 1 to pick the largest file\n");
    printf("Enter 2 to pick the smallest file\n");
    printf("Enter 3 to specify the name of a file\n");

    // Get user input
    int choice;
    printf("\nEnter a choice from 1 to 3: ");
    scanf("%d", &choice);

    // Validate the user input
    if (choice < 1 || choice > 3) {
        printf("Invalid choice! Please try again.\n\n");
        get_input_file_name(file_name);
        return;
    }

    // Get the file name to be processed
    if (choice == 1) {
        get_max_file_name(file_name);
        return;
    } else if (choice == 2) {
        get_min_file_name(file_name);
        return;
    } else if (choice == 3) {
        // Get the file name from the user
        printf("Enter the complete file name: ");
        scanf("%s", file_name);
        // Check if the file exists
        struct stat st;
        if (stat(file_name, &st) == -1) {
            printf("Error: Failed to open file %s\n", file_name);
            get_input_file_name(file_name);
            return;
        }
    }
}


void file_selection_menu() {

    // Get the file name to be processed
    char * file_name;
    file_name = malloc(sizeof(char) * 256);
    get_input_file_name(file_name);

    printf("Now processing the chosen file named %s\n", file_name);

    // Read the file and create a linked list
    struct movie_list_t * head = read_movies(file_name);

    // Create a folder with a random name
    char * folder_name = make_folder();
    printf("Created directory with the name: %s\n", folder_name);

    // Write the movies to the files
    write_years(head, folder_name);

    // Set the permissions of the folder and files
    set_permissions(folder_name);
}

void main_loop() {

    // Display menu
    printf("\n1. Select file to process\n");
    printf("2. Exit the program\n");
    printf("\nEnter a choice 1 or 2: ");

    // Get user input
    int choice;
    scanf("%d", &choice);

    // Validate the user input
    if (choice == 1) {
        file_selection_menu();
        main_loop();
    } else if (choice == 2) {
        return;
    } else {
        printf("Invalid choice! Please try again.\n");
        main_loop();
    }
}

int main(int argc, char** argv) {

    // Start the main loop
    main_loop();

    return 0;
}