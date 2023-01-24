
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

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

void print_movies_by_year(struct movie_list_t * head, int year) {
    int printed_movies = 0;
    while (head != NULL) {
        assert(head->movie != NULL);
        if (head->movie->year == year) {
            printf("%s\n", head->movie->title);
            printed_movies++;
        }
        head = head->next;
    }
    if (printed_movies == 0) {
        printf("No data about movies released in the year %d\n", year);
    }
}

int highest_rated_in_year(struct movie_list_t * head, int year) {
    int highest_rated = 0;
    int i = 0;
    int highest_rated_idx = 0;
    while (head != NULL) {
        assert(head->movie != NULL);
        if (head->movie->year == year) {
            if (head->movie->rating > highest_rated) {
                highest_rated = head->movie->rating;
                highest_rated_idx = i;
            }
        }
        i++;
        head = head->next;
    }
    // return highest_rated;
    return highest_rated_idx;
}

void print_movies_by_highest_rated_per_year(struct movie_list_t * head) {
    struct movie_list_t * current = head;
    int printed_movies = 0;
    int i = 0;
    while (current != NULL) {
        assert(current->movie != NULL);
        int year = current->movie->year;
        int highest_rated_idx = highest_rated_in_year(head, year);
        if (i == highest_rated_idx) {
            printf("%d %.1f %s\n", current->movie->year, current->movie->rating, current->movie->title);
            printed_movies++;
        }
        i++;
        current = current->next;
    }
    if (printed_movies == 0) {
        printf("No movies present.\n");
    }
}

void print_movies_in_language(struct movie_list_t * head, char* language) {
    int printed_movies = 0;
    while (head != NULL) {
        assert(head->movie != NULL);
        for (int i = 0; i < 5; i++) {
            if (strcmp(head->movie->languages[i], language) == 0) {
                printf("%d %s\n", head->movie->year, head->movie->title);
                printed_movies++;
                break;
            }
        }
        head = head->next;
    }
    if (printed_movies == 0) {
        printf("No movies in %s.\n", language);
    }
}

void menu_loop(struct movie_list_t * head) {
    
    printf("1. Show movies released in the specified year\n");
    printf("2. Show highest rated movie for each year\n");
    printf("3. Show the title and year of release of all movies in a specific language\n");
    printf("4. Exit from the program\n");

    int choice = 0;
    printf("\nEnter your choice: ");
    scanf("%d", &choice);

    if (choice < 1 || choice > 4) {
        printf("\nInvalid choice. Try again.\n\n");
        menu_loop(head);
        return;
    }

    if (choice == 4) {
        printf("\nGoodbye.\n");
        return;
    }

    if (choice == 1) {
        printf("Enter a year: ");
        int year = 0;
        scanf("%d", &year);
        print_movies_by_year(head, year);
        printf("\n");
        menu_loop(head);
        return;
    }

    if (choice == 2) {
        print_movies_by_highest_rated_per_year(head);
        printf("\n");
        menu_loop(head);
        return;
    }

    if (choice == 3) {
        printf("Enter a language: ");
        char language[20];
        scanf("%s", language);
        print_movies_in_language(head, language);
        printf("\n");
        menu_loop(head);
        return;
    }
}



int main(int argc, char** argv) {

    printf("Hello.\n");

    if (argc < 2) {
        printf("Error: Missing file argument\n");
        return 1;
    }

    char* file_name = argv[1];

    // Open the file
    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        printf("Error: Failed to open file %s\n", file_name);
        return 1;
    }

    char* line = NULL;
    int line_length = 0;

    struct movie_list_t * head = NULL;

    while (getline(&line, &line_length, file) != -1) {
        // line_length = strlen(line);
        line[strcspn(line, "\r\n")] = 0;
        // printf("(length %d) '%s'\n", line_length, line);

        struct movie_t movie = construct_movie(line);
        head = append(head,movie);
        // if (movie.title != NULL) { free(movie.title); }

        // free(line);
    }
    // free(line);
    fclose(file);
    
    if (head->next != NULL) {
        head = head->next;
    } else {
        printf("No movies present.\n");
        return 0;
    }

    print_movie_list(head);
    int movie_count = list_length(head);
    printf("Processed file %s and parsed data for %d movies.\n", file_name, movie_count);

    menu_loop(head);

    return 0;
}