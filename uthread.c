#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <ucontext.h>
#include "uthread.h"
#include "list_head.h"
#include "types.h"

/* You can include other header file, if you want. */

/*******************************************************************
 * struct tcb
 *
 * DESCRIPTION
 *    tcb is a thread control block.
 *    This structure contains some information to maintain thread.
 *
 ******************************************************************/
struct tcb {
    struct list_head list;
    ucontext_t *context;
    enum uthread_state state;
    int tid;

    int lifetime; // This for preemptive scheduling
    int priority; // This for priority scheduling
};

/***************************************************************************************
 * LIST_HEAD(tcbs);
 *
 * DESCRIPTION
 *    Initialize list head of thread control block.
 *
 **************************************************************************************/
LIST_HEAD(tcbs);
int n_tcbs = 0;
struct ucontext_t *t_context;

int g_policy;
struct tcb *running;

/***************************************************************************************
 * next_tcb()
 *
 * DESCRIPTION
 *
 *    Select a tcb with current scheduling policy
 *
 **************************************************************************************/
void next_tcb() {

    struct tcb *next = malloc(sizeof(struct tcb));
    struct tcb *current = running;

    switch(g_policy)
    {
        case 0: // FIFO
            next = fifo_scheduling(running);
            break;
        case 1: //RR 
            next = rr_scheduling(running);
            break;
        case 3: // prio
            next = prio_scheduling(running);
            break;
        case 4: // sjf
            next = sjf_scheduling(running);
            break;
    }
    running = next;
    fprintf(stderr, "SWAP %d -> %d\n",current->tid, running -> tid);
    swapcontext(current->context, running->context);
    
}

/***************************************************************************************
 * struct tcb *fifo_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using First-In-First-Out policy
 *
 **************************************************************************************/
struct tcb *fifo_scheduling(struct tcb *next) {
    
    struct tcb *run = malloc(sizeof(struct tcb));
    list_for_each_entry_reverse(run, &tcbs, list)
    {
        if(run->tid==next->tid) 
        {
            run->tid = 2; //terminated
            break;
        }
    }

    struct tcb *temp = malloc(sizeof(struct tcb));
    list_for_each_entry_reverse(temp, &tcbs, list)
    {
        if(temp->tid != next->tid && temp->state == 0)
        {
            //fprintf(stderr,"temp tid : %d\n",temp->tid);
            temp->state = 1;//running
            n_tcbs--;
            //fprintf(stderr, "n_tcbs : %d sche tid : %d state : %d\n",n_tcbs ,temp->tid,temp->state);
            return temp;
        }
    }
    
}

/***************************************************************************************
 * struct tcb *rr_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using round robin policy
 *
 **************************************************************************************/
struct tcb *rr_scheduling(struct tcb *next) {

    /* TODO: You have to implement this function. */
    struct tcb *temp;
    list_for_each_entry(temp,&tcbs,list)
    {
        if(temp->tid == next->tid)
        {
            if (temp->lifetime>0) temp->state=0; //ready
            else temp->state=2; //terminated 
            break;
        }
    }
    list_for_each_entry(temp,&tcbs,list)
    {
        if(temp->tid > next->tid)
        {
            temp->lifetime--;
            temp->state = 1;    //running
            return  temp;
        }
    }

}

/***************************************************************************************
 * struct tcb *prio_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using priority policy
 *
 **************************************************************************************/
struct tcb *prio_scheduling(struct tcb *next) {

    /* TODO: You have to implement this function. */

    struct tcb *temp;
    int max_prio = -1;
    int tid = -2;
    list_for_each_entry(temp, &tcbs, list)
    {
        if(temp->tid == next->tid)
        {
            if (temp->lifetime>0) temp->state=0; //ready
            else temp->state=2; //running
        }
        if(temp->priority>max_prio && temp->state == 0)
        {
            max_prio = temp->priority;
            tid = temp->tid;
        }
    }
    list_for_each_entry(temp, &tcbs, list)
    {
        if(temp->tid == tid)
        {
            temp->lifetime--;
            temp->state = 1; //running
            return temp;
        }
    }

}

/***************************************************************************************
 * struct tcb *sjf_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using shortest job first policy
 *
 **************************************************************************************/
struct tcb *sjf_scheduling(struct tcb *next) {

    /* TODO: You have to implement this function. */
    struct tcb *temp;
    int min_lifetime = -1;
    int tid = -2;
    list_for_each_entry(temp, &tcbs, list)
    {
        if(temp->tid == next->tid)
        { 
            temp->state=2;  //terminated
        }
        if(temp->lifetime<min_lifetime && temp->state==0)
        {
            min_lifetime = temp->lifetime;
            tid = temp->tid;
        }
    }
    list_for_each_entry(temp, &tcbs, list)
    {
        if(temp->tid == tid)
        {
            temp->state = 1;    //running
            return temp;
        }
    }

}

/***************************************************************************************
 * uthread_init(enum uthread_sched_policy policy)
 *
 * DESCRIPTION
 *    Initialize main tread control block, and do something other to schedule tcbs
 *
 **************************************************************************************/
void uthread_init(enum uthread_sched_policy policy) {
    
    /* TODO: You have to implement this function. */
    g_policy = policy; // store policy to golbal policy
   
    struct tcb * main = malloc(sizeof(struct tcb));
    main -> context = malloc(sizeof(struct ucontext_t));
    
    main -> state = 1; // running
    main -> tid = MAIN_THREAD_TID;
    n_tcbs++;
    main -> lifetime = MAIN_THREAD_LIFETIME;
    main -> priority = MAIN_THREAD_PRIORITY;
    
    getcontext(main->context);
    
    main -> context -> uc_link = 0;
    main -> context -> uc_stack.ss_sp = malloc(MAX_STACK_SIZE);
    main -> context -> uc_stack.ss_size = MAX_STACK_SIZE;
    
    //setcontext(main->context);
    
    list_add_tail(&main->list, &tcbs);

    running = main;
    t_context = running->context;

    /* DO NOT MODIFY THESE TWO LINES */
    __create_run_timer();
    __initialize_exit_context();
}


/***************************************************************************************
 * uthread_create(void* stub(void *), void* args)
 *
 * DESCRIPTION
 *
 *    Create user level thread. This function returns a tid.
 *
 **************************************************************************************/
int uthread_create(void* stub(void *), void* args) {
    
    /* TODO: You have to implement this function. */
    
    struct tcb *new = malloc(sizeof(struct tcb));
    new -> context = malloc(sizeof(struct ucontext_t));
    
    new -> state = 0 ; //READY
    new -> tid = *(int *)args;
    new -> lifetime = *(int *)(args+sizeof(int));
    new -> priority = *(int *)(args+sizeof(int)*2);
    
    getcontext(new->context);

    new -> context -> uc_link = t_context;
    new -> context -> uc_stack.ss_sp = malloc(MAX_STACK_SIZE);
    new -> context -> uc_stack.ss_size = MAX_STACK_SIZE;
    n_tcbs++;

    makecontext(new -> context, (void *) stub, 0);
    
    
    list_add_tail(&new->list, &tcbs);
    
    return new->tid; // tid return
}

/***************************************************************************************
 * uthread_join(int tid)
 *
 * DESCRIPTION
 *
 *    Wait until thread context block is terminated.
 *
 **************************************************************************************/
void uthread_join(int tid) {
    struct tcb *join;
    struct tcb *current;

    list_for_each_entry(join, &tcbs, list)
    {
        if(join -> tid == tid && join -> state == 2)
        {
            fprintf(stderr,"JOIN %d\n",join->tid);

            list_del(&join->list);
            free(join->context);
            free(join);
            n_tcbs--;

            break;
        }
    }
    
    
    
}

/***************************************************************************************
 * __exit()
 *
 * DESCRIPTION
 *
 *    When your thread is terminated, the thread have to modify its state in tcb block.
 *
 **************************************************************************************/
void __exit() {

    /* TODO: You have to implement this function. */
    
}

/***************************************************************************************
 * __initialize_exit_context()
 *
 * DESCRIPTION
 *
 *    This function initializes exit context that your thread context will be linked.
 *
 **************************************************************************************/
void __initialize_exit_context() {

    /* TODO: You have to implement this function. */
}

/***************************************************************************************
 *
 * DO NOT MODIFY UNDER THIS LINE!!!
 *
 **************************************************************************************/

static struct itimerval time_quantum;
static struct sigaction ticker;

void __scheduler() {
    printf("this is __scheduler()\n");
    if(n_tcbs > 1)
        next_tcb();
}

void __create_run_timer() {

    time_quantum.it_interval.tv_sec = 0;
    time_quantum.it_interval.tv_usec = SCHEDULER_FREQ;
    time_quantum.it_value = time_quantum.it_interval;

    ticker.sa_handler = __scheduler;
    sigemptyset(&ticker.sa_mask);
    sigaction(SIGALRM, &ticker, NULL);
    ticker.sa_flags = 0;

    setitimer(ITIMER_REAL, &time_quantum, (struct itimerval*) NULL);
}

void __free_all_tcbs() {
    struct tcb *temp;

    list_for_each_entry(temp, &tcbs, list) {
        if (temp != NULL && temp->tid != -1) {
            list_del(&temp->list);
            free(temp->context);
            free(temp);
            n_tcbs--;
            temp = list_first_entry(&tcbs, struct tcb, list);
        }
    }
    temp = NULL;
    free(t_context);
}