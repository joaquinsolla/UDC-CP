/**
JOAQUÍN SOLLA VÁZQUEZ - joaquin.solla@udc.es

COMPAÑERO: SAÚL LEYVA SANTARÉN - saul.leyva.santaren@udc.es

GROUP 2.5

EXERCISE 1
 */

#include <sys/types.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#define PASS_LEN 6
#define NUM_THREADS 5

void *break_process (void *ptr); //CABECERA DE FUNCION


/**GESTION DE THREADS*/
struct shared {
    bool found;
};

struct args {
    int num_thread; //starts in 0
    int min;
    int max;
    unsigned char **answer;
    char *md5;
    bool finished;
    struct shared *shared;
};

struct thread_info {
    pthread_t id;               // id returned by pthread_create()
    struct args *args;          // struct with arguments of the thread
    //pthread_mutex_t *mutex;   // mutex of thread
};

struct thread_info *start_threads(int bound, char *md5, unsigned char **answer, struct shared *shared){
    //answer is a pointer to variable answer of break_pass function
    int i;
    struct thread_info *threads;

    printf("Creating %d threads\n", NUM_THREADS);
    threads = malloc(sizeof(struct thread_info) * NUM_THREADS);

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }


    int passes_per_thread = ((26 ^ PASS_LEN)/NUM_THREADS);    //REPARTIR LOS PASSES A PROBAR ENTRE LOS THREADS

    //VARIABLES AUXILIARES
    int aux_min=0;
    int aux_max;

    // Create NUM_THREADS threads
    for(i = 0; i < NUM_THREADS; i++) {

        if(i == (NUM_THREADS-1)){
            aux_max = bound;
        }else aux_max = aux_min + passes_per_thread;

        threads[i].args = malloc(sizeof(struct args));
        threads[i].args->num_thread = i;
        threads[i].args->min = aux_min;
        threads[i].args->max = aux_max;
        threads[i].args->answer = answer;
        threads[i].args->md5 = md5;
        threads[i].args->finished = false;
        threads[i].args->shared = shared;

        if (0 != pthread_create(&threads[i].id, NULL, break_process, threads[i].args)) {
            printf("Could not create thread #%d\n", i);
            exit(1);
        }

        aux_min = aux_max;
    }

    return threads;
}

char *wait(struct thread_info *threads, char *answer, struct shared *shared) {

    // Wait for the threads to finish
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i].id, NULL);
        if (threads[i].args->finished) strcpy(answer, (char *) threads[i].args->answer);
    }

    for (int i = 0; i < NUM_THREADS; i++)
        free(threads[i].args);


    free(threads);

    return answer;
}


/**FUNCIONES PASS*/

long ipow(long base, int exp)
{
    long res = 1;
    for (;;)
    {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return res;
}

long pass_to_long(char *str) {
    long res = 0;

    for(int i=0; i < PASS_LEN; i++)
        res = res * 26 + str[i]-'a';

    return res;
}

void long_to_pass(long n, unsigned char *str) {  // str should have size PASS_SIZE+1
    for(int i=PASS_LEN-1; i >= 0; i--) {
        str[i] = n % 26 + 'a';
        n /= 26;
    }
    str[PASS_LEN] = '\0';
}

void to_hex(unsigned char *res, char *hex_res) {
    for(int i=0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(&hex_res[i*2], 3, "%.2hhx", res[i]);
    }
    hex_res[MD5_DIGEST_LENGTH * 2] = '\0';
}

void *break_process (void *ptr) {

    struct args *args = ptr;

    if (args->shared->found) return NULL;

    unsigned char res[MD5_DIGEST_LENGTH];
    char hex_res[MD5_DIGEST_LENGTH * 2 + 1];
    unsigned char *pass = malloc((PASS_LEN + 1) * sizeof(char));

    for (int i = args->min; i < args->max; i++) {

        if (args->shared->found){
            free(pass);
            return NULL;
        }

        long_to_pass(i, pass);
        MD5(pass, PASS_LEN, res);
        to_hex(res, hex_res);
        if (!strcmp(hex_res, args->md5)) {   // Found it!
            if (args->shared->found){
                free(pass);
                return NULL;
            }
            args->shared->found = true;
            args->finished = true;
            printf("PASSWORD FOUND\n");
            strcpy((char *) args->answer, (char *) pass);
            break;
        }

    }

    free(pass);

    return NULL;
}

char *break_pass(char *md5, char *aux_pass, unsigned char *answer) {
    long bound = ipow(26, PASS_LEN);    // we have passwords of PASS_LEN
    // lowercase chars =>
    // 26 ^ PASS_LEN  different cases

    struct thread_info *thrs;
    struct shared shared;
    shared.found = false;

    thrs = start_threads(bound, md5, &answer, &shared);



    aux_pass = wait(thrs, aux_pass, &shared);

    return (char *) aux_pass;
}


int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Use: %s string\n", argv[0]);
        exit(0);
    }

    unsigned char *answer = malloc((PASS_LEN + 1) * sizeof(char));  //ADDED
    char *aux = malloc(sizeof(char)*(PASS_LEN + 1));

    char *pass = break_pass(argv[1], aux, answer);


    printf("%s: %s\n", argv[1], pass);
    free(pass);
    free(answer);

    return 0;
}