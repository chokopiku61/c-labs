#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// Структура, которая будет передаваться из потока в main
typedef struct {
    char *filename;       // имя файла (указатель на argv, менять не нужно)
    long line_count;
    char *longest_line;   // строка (выделена через malloc)
    int has_error;
    char error_msg[256];  // сообщение об ошибке
} ThreadResult;

void *mythread(void *arg) {
    // arg содержит имя файла
    char *fname = (char *)arg;

    // Выделяем память под результат, который заберем в main
    ThreadResult *res = malloc(sizeof(ThreadResult));
    if (!res) {
        return NULL; // Не удалось выделить память даже под структуру
    }

    res->filename = fname;
    res->line_count = 0;
    res->longest_line = NULL;
    res->has_error = 0;
    res->error_msg[0] = '\0';

    FILE *fd;
    char buffer[4096];
    size_t max_len = 0;
    char *current_longest = NULL;
    long lines = 0;

    if ((fd = fopen(fname, "r")) == NULL) {
        res->has_error = 1;
        snprintf(res->error_msg, sizeof(res->error_msg),
                 "Cannot open file: %s (%s)", fname, strerror(errno));
        return (void *)res;
    }

    while (fgets(buffer, sizeof(buffer), fd) != NULL) {
        lines++;
        size_t len = strlen(buffer);

        // Убираем '\n'
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        // Ищем самую длинную (последнюю при равенстве)
        if (len >= max_len) {
            max_len = len;
            if (current_longest != NULL) {
                free(current_longest);
            }
            current_longest = malloc(len + 1);
            if (current_longest == NULL) {
                res->has_error = 1;
                snprintf(res->error_msg, sizeof(res->error_msg), "Memory allocation failed");
                fclose(fd);
                // Не забываем освободить то, что уже успели выделить
                if (res->longest_line) free(res->longest_line); 
                free(res);
                return NULL;
            }
            strcpy(current_longest, buffer);
        }
    }

    fclose(fd);

    res->line_count = lines;
    res->longest_line = current_longest;
    return (void *)res; // Возвращаем указатель на структуру
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 [file2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int file_count = argc - 1;
    pthread_t *threads = malloc(file_count * sizeof(pthread_t));

    if (!threads) {
        fprintf(stderr, "Memory allocation failed\n");
        return EXIT_FAILURE;
    }

    // Создаем потоки. В качестве аргумента передаем просто имя файла (argv[i+1])
    for (int i = 0; i < file_count; i++) {
        int ret = pthread_create(&threads[i], NULL, mythread, (void *)argv[i + 1]);
        if (ret != 0) {
            fprintf(stderr, "Error creating thread for file '%s'\n", argv[i + 1]);
            free(threads);
            return EXIT_FAILURE;
        }
        printf("Thread created for file: %s\n", argv[i + 1]);
    }

    printf("\n--- Results ---\n");

    // Ждем завершения потоков и получаем результат через второй аргумент pthread_join
    for (int i = 0; i < file_count; i++) {
        void *retval;
        pthread_join(threads[i], &retval);

        if (!retval) {
            // Поток вернул NULL (например, не хватило памяти под структуру)
            printf("File: %s — Error: Failed to create result structure\n", argv[i + 1]);
            continue;
        }

        ThreadResult *res = (ThreadResult *)retval;

        if (res->has_error) {
            printf("File: %s — Error: %s\n", res->filename, res->error_msg);
        } else {
            if (res->line_count == 0) {
                printf("File: %s — Lines: 0, Longest line: (empty)\n", res->filename);
            } else {
                printf("File: %s — Lines: %ld, Longest line: \"%s\"\n",
                       res->filename,
                       res->line_count,
                       res->longest_line);
            }
        }

        // Освобождаем память, выделенную внутри потока
        free(res->longest_line);
        free(res);
    }

    free(threads);
    return 0;
}
