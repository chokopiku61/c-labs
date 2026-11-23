
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

typedef struct {
    char *filename;
    long line_count;
    char *longest_line;      
    int has_error;           
} FileResult;

void *mythread(void *arg) {
    FileResult *res = (FileResult *)arg;
    FILE *fd;
    char buffer[4096];

    size_t max_len = 0;
    char *current_longest = NULL;
    long lines = 0;

    if ((fd = fopen(res->filename, "r")) == NULL) {
        res->has_error = 1;
        printf ("Cannot open file.\n");
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), fd) != NULL) {
        lines++;
        size_t len = strlen(buffer);

        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        if (len >= max_len) {
            max_len = len;
            if (current_longest != NULL) {
                free(current_longest);
            }

            current_longest = malloc(len + 1);
            if (current_longest == NULL) {
                res->has_error = 1;
                printf("Memory allocation failed for file '%s'", res->filename);
                fclose(fd);
                return NULL;
            }
            strcpy(current_longest, buffer);
        }
    }

    fclose(fd);

    res->line_count = lines;
    res->longest_line = current_longest;
    res->has_error = 0;

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return EXIT_FAILURE;
    }

    int file_count = argc - 1;
    pthread_t *threads = malloc(file_count * sizeof(pthread_t));
    FileResult *results = malloc(file_count * sizeof(FileResult));

    if (!threads || !results) {
        fprintf(stderr, "Memory allocation failed\n");
        free(threads);
        free(results);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < file_count; i++) {
        results[i].filename = argv[i + 1];
        results[i].line_count = 0;
        results[i].longest_line = NULL;
        results[i].has_error = 0;
    }

    for (int i = 0; i < file_count; i++) {
        int ret = pthread_create(&threads[i], NULL, mythread, (void *)&results[i]);
        if (ret != 0) {
            fprintf(stderr, "Error creating thread for file '%s'\n", results[i].filename);
            free(threads);
            free(results);
            return EXIT_FAILURE;
        }
        printf("Thread created for file: %s\n", results[i].filename);
    }

    for (int i = 0; i < file_count; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n--- Results ---\n");
    for (int i = 0; i < file_count; i++) {
        if (results[i].has_error) {
            
        } else {
            if (results[i].line_count == 0) {
                printf("File: %s — lines: 0, Longest line: (empty)\n", results[i].filename);
            } else {
                printf("File: %s — lines: %ld, Longest line: \"%s\"\n",
                       results[i].filename,
                       results[i].line_count,
                       results[i].longest_line);
            }
        }
    }

    for (int i = 0; i < file_count; i++) {
        free(results[i].longest_line);
    }
    free(threads);
    free(results);

    return 0;
}