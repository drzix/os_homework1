#pragma warning(disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include "sched.h"

#define print_time(job)                                                 \
        do {                                                            \
                struct time_info ti;                                    \
                ti = get_fcfs_time(job);                                \
                printf("%d %d\n", ti.tard_time, ti.resp_time);          \
                ti = get_sjf_time(job);                                 \
                printf("%d %d\n", ti.tard_time, ti.resp_time);          \
                ti = get_rr_time(job);                                  \
                printf("%d %d\n", ti.tard_time, ti.resp_time);          \
        } while (0)

static inline void solve_test()
{
        struct job_head inp;
        int job_cnt;

        scanf("%d", &job_cnt);
        inp.jobs = malloc(job_cnt * sizeof(struct job_info));
        inp.job_cnt = job_cnt;

        for (int i = 0; i < job_cnt; i++)
                scanf("%d %d", &((inp.jobs + i)->arrived),
                      &((inp.jobs + i)->amount_time));

        print_time(&inp);
        free(inp.jobs);
}

int main(int argc, char *argv)
{
        int case_cnt;

        scanf("%d", &case_cnt);
        while (case_cnt--)
                solve_test();

        return 0;
}
