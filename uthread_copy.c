
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
    enum uthread_state state;	//READY, RUNNING, TERMINATED
    int tid;	//스레드 식별번호

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
LIST_HEAD(tcbs); //threads on the system

int n_tcbs = 0; //tcb count

struct ucontext_t *t_context;


/***************************************************************************************
 * next_tcb()
 *
 * DESCRIPTION
 *
 *    Select a tcb with current scheduling policy
 *
 **************************************************************************************/
void next_tcb() {

    printf("next_tcb()\n");
    /* TODO: You have to implement this function. */
    struct tcb *prev = malloc(sizeof(struct tcb));
    prev -> context = malloc(sizeof(struct ucontext_t));
    
    list_for_each_entry_reverse(prev, &tcbs,list)
    {
    	if(prev->state == 1) // RUNNING
    	{
    		prev -> state = 0; //READY
    		break;
    	}
    }
    
    struct tcb *next = malloc(sizeof(struct tcb));
    next -> context = malloc(sizeof(struct ucontext_t));
    
    list_for_each_entry_reverse(next, &tcbs,list)
    {
    	if(next->state ==2) // TERMINATED
    	{
    		next -> state = 1; //RUNNING
    		break;
    	}
    }
    
    fprintf(stderr, "SWAP %d -> %d\n", prev->tid, next->tid);
    swapcontext(prev->context, next->context);
    
    
}


/***************************************************************************************
 * struct tcb *fifo_scheduling(struct tcb *next)
 *
 * DESCRIPTION
 *
 *    This function returns a tcb pointer using First-In-First-Out policy
 *
 **************************************************************************************/
struct tcb  *fifo_scheduling(struct tcb *next) {
    /* TODO: You have to implement this function. */
    
    printf("FIFO Scheduling\n");
    setcontext(next->context);
    struct tcb *temp =  malloc(sizeof(struct tcb));
    list_for_each_entry_reverse(temp,&tcbs,list)
    {
    	if(temp -> state == 0) //READY
    	{
    		printf("Scheduling tid : %d \n",temp->tid);
    		temp->state = 2; // TERMINATED
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
    
    t_context = malloc(sizeof(struct ucontext_t));
    getcontext(t_context);
    t_context -> uc_link = 0;
    t_context -> uc_stack.ss_sp = malloc(MAX_STACK_SIZE);
    t_context -> uc_stack.ss_size = MAX_STACK_SIZE;
    t_context -> uc_stack.ss_flags = 0;

    struct tcb *main = malloc(sizeof(struct tcb));
    INIT_LIST_HEAD(&main -> list);
    main -> state = 2; // TERMINATED
    main -> tid = MAIN_THREAD_TID;
    n_tcbs++;
    main -> lifetime = MAIN_THREAD_LIFETIME;
    main -> priority = MAIN_THREAD_PRIORITY;
    main -> context = malloc(sizeof(struct ucontext_t));
    main -> context = t_context;
    main -> context -> uc_link = t_context;
    //getcontext(main->context);
        
    switch (policy)
    {
    	case 0 : //FIFO
    		makecontext(t_context, (void *)fifo_scheduling,1,main);
    		break;	
    	case 1 : //RR
    		makecontext(t_context, (void *)rr_scheduling,1,main);
    		break;
    	case 2 : //PRIO
    		makecontext(t_context, (void *)prio_scheduling,1,main);
    		break;
    	case 3 : //SJF
    		makecontext(t_context, (void *)sjf_scheduling,1,main);
    		break;
    }
    
    swapcontext(t_context, main->context);
    //list_add(&main->list, &tcbs);
    
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
    
    INIT_LIST_HEAD(&new -> list);
    
    new -> state =0 ; //READY
    new -> tid = *(int *)args;
    n_tcbs++;
    new -> lifetime = *(int *)(args+1);
    new -> priority = *(int *)(args+2);
    
    new -> context = malloc(sizeof(struct ucontext_t));
    new -> context -> uc_link = t_context;
    new -> context -> uc_stack.ss_sp = malloc(MAX_STACK_SIZE);
    new -> context -> uc_stack.ss_size = MAX_STACK_SIZE;
    
    makecontext(new -> context, (void *) stub, 0);
    
    list_add(&new->list, &tcbs);
    
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

    /* TODO: You have to implement this function. */
    
    struct tcb *prev = malloc(sizeof(struct tcb));
    prev -> context = malloc(sizeof(struct ucontext_t));
    
    list_for_each_entry_reverse(prev, &tcbs,list)
    {
    	if(prev -> tid == tid ) 
    	{
    		prev->state = 0; // READY
    		
    		fprintf(stderr, "JOIN %d\n", prev->tid);
    		setcontext(t_context);
    		
    		list_del(&prev->list);
		free(prev->context);
		free(prev);
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
    struct tcb *temp = malloc(sizeof(struct tcb));
    list_for_each_entry_reverse(temp, &tcbs,list)
    {
    	if(temp->state == 2)
    	{
    		list_del(&temp->list);
    		free(temp->context);
    		free(temp);
    		n_tcbs--;
    		temp =  list_first_entry(&tcbs, struct tcb, list);
    	}
    }

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
    struct tcb *next = malloc(sizeof(struct tcb));
    struct tcb *prev = malloc(sizeof(struct tcb));
    list_for_each_entry_reverse(next, &tcbs,list)
    {
    	if(next -> tid == MAIN_THREAD_TID)
    		break;
    }
    list_for_each_entry_reverse(prev, &tcbs,list)
    {
    	if (prev ->  state == 1) // Runnable
    	{
    		printf("RUNNABLE CHECK");
    		prev -> state = 0; // Ready
    		next -> state = 1; // Runnable
    		break;
    	}
    }
    if(prev->tid != next->tid)
    {
	     fprintf(stderr, "SWAP %d -> %d\n", prev->tid, next->tid);	// return main
	     swapcontext(prev -> context, next -> context);
    }
}

/***************************************************************************************
 *
 * DO NOT MODIFY UNDER THIS LINE!!!
 *
 **************************************************************************************/

static struct itimerval time_quantum;
static struct sigaction ticker;

void __scheduler() {
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
