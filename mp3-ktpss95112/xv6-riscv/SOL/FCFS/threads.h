#ifndef THREADS_H_
#define THREADS_H_

#include "user/setjmp.h"

struct thread {
    void (*fp)(void *arg);
    void *arg;
    void *stack;
    void *stack_p;
    jmp_buf env;
    int ID;
    int block_fd;
    char reading;
    int buf_set;
    struct thread *previous;
    struct thread *next;

    // for mp3
    int thrdstop_context_id; // When yeild or interrupt are happening, kernel stores the thread context, this is index for that.
    int remain_execution_time; // When this variable<=0, thread should exit. measure in ticks
    int start_time; // measure in ticks
    int is_yield;
    int is_exited;
};

struct thread *thread_create(void (*f)(void *), void *arg, int execution_time_slot);
void thread_add_runqueue(struct thread *t);
void thread_yield(void);
void dispatch(void);
void schedule(void);
void thread_exit(void);
void thread_start_threading(int time_slot_size);

#endif // THREADS_H_
