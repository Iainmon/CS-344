
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

struct movie_t {
    char* title;
    int year;
    char languages[5][20];
    float rating;
};

struct movie_list_t {
    struct movie_t * movie;
    struct movie_list_t * next;
};

struct movie_list_t * get_last(struct movie_list_t * ml) {
    struct movie_list_t * curr = ml;
    if (curr == NULL) {
        curr = malloc(sizeof(struct movie_list_t));
        assert(curr != NULL);
        curr->movie = NULL;
        curr->next = NULL;
        return curr;
    }
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = NULL;
    return curr;
}

struct movie_list_t * append(struct movie_list_t * ml, struct movie_t m) {

    struct movie_list_t * head = ml;
    struct movie_list_t * last = get_last(head);
    assert(last != NULL);
    struct movie_list_t * fresh = get_last(NULL); // Creates a blank list node
    assert(fresh != NULL);

    last->next = fresh;

    if (last->movie == NULL) {
        free(last);
        head = fresh;
    }

    struct movie_t * movie = malloc(sizeof(struct movie_t));
    assert(movie != NULL);
    *movie = m;
    // movie->title = NULL;
    movie->title = malloc(strlen(m.title) + 1);
    strcpy(movie->title, m.title);
    
    fresh->movie = movie;
    return head;
}


char** tokenize(char* line, char* delim, int * length) {

    int num = 0;
    
    char* rest = NULL;
    char* token;
    char* line_ = calloc(strlen(line) + 1,sizeof(char));
    strcpy(line_,line);

    token = strtok_r(line_, delim, &rest);
    while (token != NULL) {
        num++;
        token = strtok_r(NULL, delim, &rest);
    }

    char** tokens = malloc(sizeof(char) * (num + 1));

    int i = 0;
    token = strtok(line, delim);
    while (i < num) {
        tokens[i] = token;
        token = strtok(NULL,delim);
        i++;
    }
    *length = i;

    // int i = 0;
    // char* _rest = NULL;
    // token = strtok(line, delim);
    // while (token != NULL) {
    //     // printf("%d ) token: %s\n", i, token);
    //     int token_length = strlen(token);
    //     tokens[i] = token;// malloc((token_length + 1) * sizeof(char));
    //     // assert(new_token != NULL);
    //     // memset(token, '\0', token_length);
    //     // strcpy(tokens[i],token);
    //     // strncpy(new_token, token,token_length);
    //     printf("%d ) token: %s\n", i, tokens[i]);

    //     // tokens[i] = new_token;

    //     i++;
    //     token = strtok(NULL, delim);
    // }
    // // assert(i == num);
    // *length = num;

    return tokens;
}

void print_movie(struct movie_t * movie) {
    printf(
        "movie { title = '%s', year = %d, rating = %.1f, languages = ",
        movie->title,
        movie->year,
        movie->rating);
    printf("[");
    for (int i = 0; i < 5; i++) {
        if (movie->languages[i][0] == 0) { continue; }
        printf("%s %s ", (i > 0 ? "," : ""), movie->languages[i]);
    }
    printf("]\n");
}

void populate_languages(char * lang_str, struct movie_t * movie /* char (*languages)[5][20]*/) {
    int length = strlen(lang_str);
    int num = 0;
    int i = 0;
    int j = 0;
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



struct movie_t construct_movie(char* line) {

    // struct movie_t* movie = malloc(sizeof(struct movie_t));
    struct movie_t movie;

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 20; j++) {
            movie.languages[i][j] = 0;
        }
    }

    int num_tokens;
    char** tokens = tokenize(line, ",", &num_tokens);

    for (int i = 0; i < num_tokens; i++) {
        printf("%d ] token: %s \n", i, tokens[i]);
    }


    movie.title = tokens[0];
    movie.year = (int) atof(tokens[1]);
    populate_languages(tokens[2], &movie);
    movie.rating = atof(tokens[3]);

    print_movie(&movie);

    return movie;
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
        printf("(length %d) '%s'\n", line_length, line);

        struct movie_t movie = construct_movie(line);
        head = append(head,movie);
        // if (movie.title != NULL) { free(movie.title); }

        // free(line);
    }
    free(line);
    fclose(file);

    return 0;
}