#include "kernel/types.h"
#include "user/user.h"
#include "user/threads.h"

#define NULL 0

int startTime = 0;
int nowTime = -1;
int main_id = 0;
int flag = 0;
int thrdstop_test_time = 0;
int tmpTime;
void thrdstop_test1()
{
    thrdstop_test_time = uptime();
    printf("thrdstop test1 at %d\n", thrdstop_test_time-startTime);
    flag += 1;
    thrdresume(main_id, 0);
}


void gg(int *k)
{
    *k = *k + 1;
}

void test1()
{
    startTime = uptime();
    main_id = thrdstop( 5, -1, thrdstop_test1);
    int i = 0, j = 0;
    int tmp_time = 0;
    while (1)
    {   
        gg(&j);
        i ++;
        if(flag == 1)
        {
            if( thrdstop_test_time-startTime > 6 || thrdstop_test_time-startTime < 4 )
            {
                printf("FAILED, thrdstop counts down incorrectly\n");
                exit(0);
            }
            thrdstop( 10, main_id, thrdstop_test1);
            tmp_time = uptime();
            printf("tmp_time=%d\n", tmp_time);
            flag += 1;
        }
        else if( flag == 2 && uptime() - tmp_time >= 2 )
        {
            int time_consumed = cancelthrdstop( -1 );
            printf("time_consumed = %d, it should be 2\n", time_consumed);
            thrdstop( 1, main_id, thrdstop_test1);
            flag += 1;
        }
        else if( flag == 4 )
        {
            if( thrdstop_test_time-startTime > 9 || thrdstop_test_time-startTime < 7 )
            {
                printf("FAILED, thrdstop counts down incorrectly\n");
                exit(0);
            }            
            thrdstop( 10, main_id, thrdstop_test1);
            flag += 1;
        }
        else if( flag == 5 )
        {
            // just want to call uptime() many times.

            tmpTime = uptime();
            if( tmpTime != nowTime && flag == 5 )
            {
                printf("ttt %d\n", tmpTime-startTime);
                nowTime = tmpTime;
            }
            
        }
        else if( flag == 6 )
        {
            if( thrdstop_test_time-startTime > 19 || thrdstop_test_time-startTime < 17 )
            {
                printf("FAILED, thrdstop counts down incorrectly\n");
                exit(0);
            }           
            else if( i != j) printf("FAILED, i should equal to j\n");
            else printf("PASS 1\n");
            return;
        }
    }
}


void f1( void *k )
{
    int *kk = (int *)k;
    *kk = *(int *)k + 1;
    if( *kk <= 100 )
    {
        struct thread *t1 = thread_create(f1, k, 3);
        thread_add_runqueue(t1);
    }    
    thread_exit();
}

void test2()
{
    int test2k = 0;
    struct thread *t1 = thread_create(f1, &test2k, 3);
    thread_add_runqueue(t1);
    thread_start_threading(5);
    printf("test2k=%d\n", test2k);
    printf("PASS 2\n", test2k);
}

int main(int argc, char **argv)
{
   
    test1();
    test2();
    exit(0);
}
