#define _POSIX_C_SOURCE 200809L

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
    char *buffer = NULL;
    size_t capacity = 0;
    ssize_t bytes_read;

    size_t max_len = 0;
    char *cur_longest = NULL;
    long lines = 0;

    if ((fd = fopen(res->filename, "r")) == NULL) {
        res->has_error = 1;
        fprintf(stderr, "Cant open %s\n", res->filename);
        return NULL;
    }

    while ((bytes_read = getline(&buffer, &capacity, fd)) != -1) {
        lines++;
        size_t len = bytes_read;

        if (len > 0 && buffer[len - 1] == '\n') {
            len--;
            if (len > 0 && buffer[len - 1] == '\r') {
                len--;
            }
        }

        if (len >= max_len) {
            char *longest = realloc(cur_longest, len + 1);
            if (longest == NULL) {
                res->has_error = 1;
                fprintf(stderr, "Error malloc: %s\n", res->filename);
                break;
            }
            cur_longest = longest;
            memcpy(cur_longest, buffer, len);
            cur_longest[len] = '\0';
            max_len = len;
        }
    }

    if (!res->has_error && (ferror(fd) || !feof(fd))) {
        res->has_error = 1;
        fprintf(stderr, "Cant read %s\n", res->filename);
        }
    free(buffer);
    fclose(fd);

    res->line_count = lines;
    res->longest_line = cur_longest;
    return NULL;
}



int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: no input files\n");
        return EXIT_FAILURE;
    }

    int status = 0;
    int f_cnt = argc - 1;
    pthread_t *threads = malloc(f_cnt * sizeof(pthread_t));
    FileResult *results = malloc(f_cnt * sizeof(FileResult));

    if (!threads || !results) {
            fprintf(stderr, "Error malloc\n");
        free(threads);
        free(results);
        return EXIT_FAILURE;
    }   

    for (int i = 0; i < f_cnt; i++) {
        results[i].filename = argv[i + 1];
        results[i].line_count = 0;
        results[i].longest_line = NULL;
        results[i].has_error = 0;
    }

    for (int i = 0; i < f_cnt; i++) {
        int ret = pthread_create(&threads[i], NULL, mythread, (void *)&results[i]);
        if (ret != 0) {
            fprintf(stderr, "Error pthread_create: %s\n", results[i].filename);
            status = EXIT_FAILURE;
            break;
        }
        printf("Thread creat: %s | %ld\n", argv[i + 1], threads[i]);
    }

    for (int i = 0; i < f_cnt; i++) {
            pthread_join(threads[i], NULL);
    }



    printf("\n#Results \n");
    for (int i = 0; i < f_cnt; i++) {
        if (results[i].has_error) {
            status = EXIT_FAILURE;
        } else {
                printf("File: %s | lines: %ld | longest: '%s'\n", results[i].filename,  results[i].line_count, results[i].longest_line); 
        }
    }


    for (int i = 0; i < f_cnt; i++) {
        free(results[i].longest_line);
    }


    free(threads);
    free(results);

    return status;
}
