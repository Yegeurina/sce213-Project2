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
struct tcb *m_thread;
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
        case 0 :
            next = fifo_scheduling(running);
            break;
        case 1 :
            next = rr_scheduling(running);
            break;
        case 2 : 
            next = prio_scheduling(running);
            break;
        case 3 : 
            next = sjf_scheduling(running);
            break;
    }
    running = next;
    fprintf(stderr,"SWAP %d -> %d\n",current->tid, running -> tid);
    if (swapcontext(current->context, running->context)==-1)
        printf("cur : %d run : %d swapcontext() error\n",current->tid,running->tid);
    sigprocmask(SIG_BLOCK,&running->context->uc_sigmask,NULL);
       
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
    
    //printf("next tid : %d state : %d\n",next->tid, next->state);

    struct tcb *run = malloc(sizeof(struct tcb));
    list_for_each_entry(run, &tcbs, list)
    {
        //printf("run tid : %d\n",run->tid);
        if(run->tid==next->tid && run->state == 1) 
        {
            run->state = 2; //run -> terminated
            break;
        }
    }

    struct tcb *is_last;
    is_last = list_last_entry(&tcbs, struct tcb, list);
    if(next->tid == is_last->tid)
    {
        //printf("is last entry\n");
        struct tcb *main = list_first_entry(&tcbs, struct tcb, list);
        main -> state =1;
        return main;
    }

    struct tcb *temp = malloc(sizeof(struct tcb));
    list_for_each_entry(temp, &tcbs, list)
    {
        if(temp->state == 0)
        {
            temp->state = 1;//running
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
    //printf("\nnext tid : %d lifetime : %d state : %d\n",next->tid,next->lifetime, next->state);
    struct tcb *run = malloc(sizeof(struct tcb));
    list_for_each_entry(run, &tcbs, list)
    {
        //printf("run tid : %d lifetime : %d state : %d\n",run->tid,run->lifetime,run->state);
        if(run->tid == next->tid)
        {
            if (run->lifetime>0) run->state=0; //ready
            else run->state=2; //running
        }
    }

    struct tcb *is_last;
    is_last = list_last_entry(&tcbs, struct tcb, list);
    if(next->tid == is_last->tid)
    {
        //printf("is last entry\n");
        struct tcb *main = list_first_entry(&tcbs, struct tcb, list);
        main -> state =1;
        return main;
    }

    struct tcb *temp = malloc(sizeof(struct tcb));
    list_for_each_entry(temp, &tcbs, list)
    {
        //printf("temp tid : %d lifetime : %d state : %d\n",temp->tid,temp->lifetime,temp->state);
        if(temp->state == 0 && temp->tid > next->tid && temp->lifetime>0)
        {
            temp->lifetime--;
            temp->state = 1;//running
            return temp;
        }
    }
    struct tcb *main = list_first_entry(&tcbs, struct tcb, list);
    main -> state =1;
    return main;
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
    //printf("next tid : %d, state : %d\n",next->tid, next->state);
    struct tcb *temp;
    int max_prio = MAIN_THREAD_PRIORITY;
    int tid = MAIN_THREAD_TID;
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
    //printf("choiced tid : %d , max_prio : %d\n",tid, max_prio);
    if(tid == MAIN_THREAD_TID)
    {
        struct tcb *main = list_first_entry(&tcbs, struct tcb, list);
        main -> state =1;
        return main;
    }
    list_for_each_entry(temp, &tcbs, list)
    {   
        //printf("temp tid : %d, lifetime : %d, state:%d\n",temp->tid, temp->lifetime, temp->state);
        if(temp->tid == tid)
        {
            //printf("find choiced tid\n");
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
    //printf("next tid : %d next state : %d\n",next->tid, next->state);
    struct tcb *temp;
    int min_lifetime = MAIN_THREAD_LIFETIME;
    int tid = MAIN_THREAD_TID;
    list_for_each_entry(temp, &tcbs, list)
    {
        if(temp->tid == next->tid)
        { 
            temp->state=2;  //terminated
        }
        if(temp->lifetime < min_lifetime && temp->state==0)
        {
            min_lifetime = temp->lifetime;
            tid = temp->tid;
        }
    }
    if (tid == MAIN_THREAD_TID)
    {
        struct tcb *main = list_first_entry(&tcbs, struct tcb, list);
        main -> state =1;
        return main;
    }
    //printf("min_lifetime tid : %d\n",tid);
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
    //printf("g_policy : %d\n",g_policy);
   
    struct tcb *main = malloc(sizeof(struct tcb));
    main -> context = malloc(sizeof(struct ucontext_t));
    
    main -> state = 1; // running
    main -> tid = MAIN_THREAD_TID;
    n_tcbs++;
    main -> lifetime = MAIN_THREAD_LIFETIME;
    main -> priority = MAIN_THREAD_PRIORITY;
    
    if (getcontext(main->context)==-1)
        printf("main : getcontext() Error\n");
    
    main -> context -> uc_link = NULL;
    main -> context -> uc_stack.ss_sp = malloc(MAX_STACK_SIZE);
    main -> context -> uc_stack.ss_size = MAX_STACK_SIZE;
    
    list_add_tail(&main->list, &tcbs);

    running = main;
    m_thread = main;
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
    
    if(getcontext(new->context)==-1)
        printf("tid  %d : getcontext() error",new->tid);

    new -> context -> uc_link = t_context;
    new -> context -> uc_stack.ss_sp = malloc(MAX_STACK_SIZE);
    new -> context -> uc_stack.ss_size = MAX_STACK_SIZE;

    switch(g_policy)
    {
        case 1 :
        case 2 :
            sigemptyset(&new -> context ->uc_sigmask);
            sigaddset(&new -> context ->uc_sigmask,SIGINT);
            sigaddset(&new -> context ->uc_sigmask,SIGQUIT);
            break;
        default :
            sigfillset(&new -> context ->uc_sigmask);
    }

    makecontext(new -> context, (void *) stub, 0);
       
    list_add_tail(&new->list, &tcbs);
    n_tcbs++;

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
        if(join -> tid == tid )
        {
            while(join -> state != 2);

            fprintf(stderr,"JOIN %d\n",join->tid);

            /*list_del(&join->list);
            free(join->context);
            free(join);
            n_tcbs--;*/

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
    //printf("this is __scheduler()\n");
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