/**
JOAQUÍN SOLLA VÁZQUEZ - joaquin.solla@udc.es

GROUP 2.5

EXERCISE 3
*/

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#include "options.h"

#define MAX_AMOUNT 20

int thr_cand = 0, withdraw_waiting = 0;

struct bank {
    int num_accounts;        // number of accounts
    int *accounts;           // balance array
    pthread_mutex_t *mutex;  // mutex array
    pthread_cond_t cannot_withdraw;
};

struct args {
    int		thread_num;   // application defined thread #
    int		delay;	      // delay between operations
    int		iterations;   // number of operations
    int     net_total;           // total amount deposited by this thread
    int     net_wdraw;           // total amount withdrawed by this thread (added)
    struct bank *bank;          // pointer to the bank (shared with other threads)
};

struct thread_info {
    pthread_t    id;        // id returned by pthread_create()
    struct args *args;      // pointer to the arguments
};


// Threads run on this function
void *transfer(void *ptr)
{
    struct args *args =  ptr;
    int amount, giving, receiving, balance_giv, balance_rec;

    while(args->iterations--) {
        giving = rand() % args->bank->num_accounts;
        do {receiving = rand() % args->bank->num_accounts;} while (receiving == giving); // transer to different account

        if(giving < receiving){
            pthread_mutex_lock(&args->bank->mutex[giving]);
            pthread_mutex_lock(&args->bank->mutex[receiving]);
        }
        else{
            pthread_mutex_lock(&args->bank->mutex[receiving]);
            pthread_mutex_lock(&args->bank->mutex[giving]);
        }

        if(args->bank->accounts[giving] == 0) amount = 0; // there is no money to transfer
        else amount  = rand() % args->bank->accounts[giving];

        printf("Thread %d transfering %d from account %d (%d) to account %d (%d)\n",
               args->thread_num, amount, giving, args->bank->accounts[giving], receiving, args->bank->accounts[receiving]);

        balance_giv = args->bank->accounts[giving]; // Substract amount from giving
        if(args->delay) usleep(args->delay);

        balance_giv -= amount;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[giving] = balance_giv;
        if(args->delay) usleep(args->delay);

        balance_rec = args->bank->accounts[receiving]; //Add amount to reciving
        if(args->delay) usleep(args->delay);

        balance_rec += amount;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[receiving] = balance_rec;

        thr_cand--;
/*
        if(withdraw_waiting>0){
            printf("-------------START BROADCAST (thread %d)\n", args->thread_num);
            pthread_cond_broadcast(&args->bank->cannot_withdraw);
            printf("-------------END BROADCAST (thread %d)\n", args->thread_num);
        }
*/
        pthread_mutex_unlock(&args->bank->mutex[giving]);
        pthread_mutex_unlock(&args->bank->mutex[receiving]);
    }

    return NULL;
}

// Threads run on this function
void *deposit(void *ptr)
{
    struct args *args =  ptr;
    int amount, account, balance;

    while(args->iterations--) {
        amount  = rand() % MAX_AMOUNT;
        account = rand() % args->bank->num_accounts;

        pthread_mutex_lock(&args->bank->mutex[account]);

        printf("Thread %d depositing %d on account %d\n",
               args->thread_num, amount, account);

        balance = args->bank->accounts[account];
        if(args->delay) usleep(args->delay); // Force a context switch

        balance += amount;
        if(args->delay) usleep(args->delay);

        args->bank->accounts[account] = balance;
        if(args->delay) usleep(args->delay);

        args->net_total += amount;

        thr_cand--;
/*
        if(withdraw_waiting>0){
            printf("-------------START BROADCAST (thread %d)\n", args->thread_num);
            pthread_cond_broadcast(&args->bank->cannot_withdraw);
            printf("-------------END BROADCAST (thread %d)\n", args->thread_num);
        }
*/
        pthread_mutex_unlock(&args->bank->mutex[account]);
    }
    return NULL;
}

// Threads run on this function
void *withdraw(void *ptr){
    struct args *args =  ptr;
    int amount, account, balance;
    bool new = true;

    amount = rand() % MAX_AMOUNT;
    account = rand() % args->bank->num_accounts;

    pthread_mutex_lock(&args->bank->mutex[account]);

    while (args->bank->accounts[account] < amount) {
        //SI NO HAY MAS OPCIONES NO RETIRA
        if (thr_cand <= 0) {
            printf("Thread %d COULD NOT withdraw %d of account %d (not enough money)\n",
                   args->thread_num, amount, account);
            withdraw_waiting--;
            pthread_mutex_unlock(&args->bank->mutex[account]);
            return NULL;

        } else {
            //DUERME:
            if (new) {
                new = false;
                withdraw_waiting++;
            }
            if(0 != pthread_cond_wait(&args->bank->cannot_withdraw, &args->bank->mutex[account])){
                printf("--- Wait error ---\n");
                exit(1);

            } //else printf("WAITING THREAD %d,   THR_CAND RESTANTES: [%d]\n", args->thread_num, thr_cand);
            //printf("THREAD %d WOKE UP\n", args->thread_num);

        }
    }

    /**-------SE CUMPLEN LAS CONDICIONES (SE RETIRA DINERO):--------*/
    printf("Thread %d withdrawing %d of account %d\n",
           args->thread_num, amount, account);

    balance = args->bank->accounts[account];
    if (args->delay) usleep(args->delay); // Force a context switch

    balance -= amount;
    if (args->delay) usleep(args->delay);

    args->bank->accounts[account] = balance;
    if (args->delay) usleep(args->delay);

    args->net_wdraw += amount;

    /**-------------------------------------------------------------*/

    if (!new) withdraw_waiting--;

    pthread_mutex_unlock(&args->bank->mutex[account]);

    return NULL;
}

// start opt.num_threads threads running on deposit, transfer and withdraw.
struct thread_info *start_threads(struct options opt, struct bank *bank)
{
    int i;
    struct thread_info *threads;

    printf("creating %d threads for deposits\n", opt.num_threads);
    printf("creating %d threads for tranfers\n", opt.num_threads);
    printf("creating %d threads for withdraws\n", opt.num_threads);
    threads = malloc(sizeof(struct thread_info) * opt.num_threads * 3);

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running swap()
    for(i = 0; i < (opt.num_threads*3); i++){
        threads[i].args = malloc(sizeof(struct args));

        threads[i].args -> thread_num = i;
        threads[i].args -> net_total  = 0;
        threads[i].args -> net_wdraw  = 0;
        threads[i].args -> bank       = bank;
        threads[i].args -> delay      = opt.delay;
        threads[i].args -> iterations = opt.iterations;

        if (0 != pthread_mutex_init(&threads[i].args->bank->mutex[i], NULL)) { // initializes the mutex
            printf("Mutex init failed\n");
            exit(1);
        }
    }

    thr_cand = opt.num_threads*2*opt.iterations;

    // deposit threads
    for (i = 0; i < opt.num_threads; i++) {
        if (0 != pthread_create(&threads[i].id, NULL, deposit, threads[i].args)) {
            printf("Could not create thread #%d\n", i);
            exit(1);
        }
    }

    // transfer threads
    for (i = opt.num_threads; i < (opt.num_threads*2); i++) {
        if (0 != pthread_create(&threads[i].id, NULL, transfer, threads[i].args)) {
            printf("Could not create thread #%d\n", i);
            exit(1);
        }
    }

    // withdraw threads
    for (i = (opt.num_threads*2); i < (opt.num_threads*3); i++) {
        if (0 != pthread_create(&threads[i].id, NULL, withdraw, threads[i].args)) {
            printf("Could not create thread #%d\n", i);
            exit(1);
        }
    }

    return threads;
}

// Print the final balances of accounts and threads
void print_balances(struct bank *bank, struct thread_info *thrs, int num_threads) {
    int total_deposits=0, total_withdraws=0 ,bank_total=0;

    printf("\nNet deposits by thread\n");
    for(int i=0; i < num_threads; i++) {
        printf("%d: %d\n", i, thrs[i].args->net_total);
        total_deposits += thrs[i].args->net_total;
    }
    printf("Total: %d\n", total_deposits);

    printf("\nNet withdraws by thread\n");
    for (int i = (num_threads*2); i < (num_threads*3); i++) {
        printf("%d: %d\n", i, thrs[i].args->net_wdraw);
        total_withdraws += thrs[i].args->net_wdraw;
    }
    printf("Total: %d\n", total_withdraws);

    printf("\nAccount balance\n");
    for(int i=0; i < bank->num_accounts; i++) {
        printf("%d: %d\n", i, bank->accounts[i]);
        bank_total += bank->accounts[i];
    }
    printf("Total: %d\n", bank_total);

}

// wait for all threads to finish, print totals, and free memory
void wait(struct options opt, struct bank *bank, struct thread_info *threads) {

    // Wait for the threads to finish
    for (int i = 0; i < opt.num_threads*3; i++)
        pthread_join(threads[i].id, NULL);


    print_balances(bank, threads, opt.num_threads);

    for (int i = 0; i < opt.num_threads*3; i++)
        free(threads[i].args);


    free(threads);
    free(bank->accounts);
    free(bank->mutex);
}

// allocate memory, and set all accounts to 0
void init_accounts(struct bank *bank, int num_accounts) {
    bank->num_accounts = num_accounts;
    bank->accounts     = malloc(bank->num_accounts * sizeof(int));
    bank->mutex        = malloc(bank->num_accounts * sizeof(pthread_mutex_t));

    for(int i=0; i < bank->num_accounts; i++)
        bank->accounts[i] = 0;
}

int main (int argc, char **argv)
{
    struct options      opt;
    struct bank         bank;
    struct thread_info *thrs;

    srand(time(NULL));

    // Default values for the options
    opt.num_threads  = 5;
    opt.num_accounts = 10;
    opt.iterations   = 100;
    opt.delay        = 10;

    read_options(argc, argv, &opt);

    init_accounts(&bank, opt.num_accounts);

    thrs = start_threads(opt, &bank);

    wait(opt, &bank, thrs);

    return 0;
}
