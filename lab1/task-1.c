#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void *mythread(void *fname)
{
       FILE *fd;

       char ch;

       char *fn;
       pthread_t mythid;

       mythid = pthread_self();

       fn=(char *)fname;
       printf("File %s:\n", fn);

       if((fd = fopen (fn, "r"))==NULL)
       {
             printf ("Cannot open file.\n");

             exit(-1);
       }

       while (fscanf(fd, "%c", &ch) == 1) {
        printf("Symbol %c\n", ch);
       }

       fclose (fd);

       printf("Thread %ld\n\n", mythid);

       return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t mythid;
    mythid = pthread_self();
    pthread_t *threads = malloc((argc - 1) * sizeof(pthread_t));

    for (int i = 1; i < argc; i++) {
        int result;
        result = pthread_create( &threads[i-1], NULL, mythread, (void *)argv[i]);
 
        if(result != 0) {
            printf ("Error on thread create, return value = %d\n", result);
            free(threads);
            exit(-1);
        }
 
        printf("Thread created, thid = %ld\n", threads[i-1]);
        
       
    }

    for (int i = 1; i < argc; i++) {
        pthread_join(threads[i-1], NULL);
    }
    printf("Thread %ld\n", mythid);
    free(threads);
    return 0;
}
 