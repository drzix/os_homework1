#ifndef _SCHED_H
#define _SCHED_H

#include "rbtree.h"

typedef int                     sched_time_t;

struct job_head {
        struct job_info                 *jobs;
        int                             job_cnt;
};

struct job_info {
        sched_time_t                    arrived;
        sched_time_t                    amount_time;
};

struct time_info {
        sched_time_t                    tard_time;
        sched_time_t                    resp_time;
};

extern struct time_info get_fcfs_time(const struct job_head *head);
extern struct time_info get_sjf_time(const struct job_head *head);
extern struct time_info get_rr_time(const struct job_head *head);

static inline void sched_job(struct time_info *info,
                             const int now,
                             const struct job_info *job)
{
        sched_time_t resp_time = now - job->arrived;

        info->resp_time += resp_time;
        info->tard_time += resp_time + job->amount_time;
}

struct wait_job {
        const struct job_info           *job;
        int                             idx;
        struct rb_node                  wait_node;
};

static inline struct wait_job *get_shortest_job(struct rb_root *root)
{
        return container_of(rb_first(root), struct wait_job, wait_node);
}

extern void push_wait_job(struct rb_root *root, struct job_info *job, int idx);
extern void pop_wait_job(struct wait_job *wjob, struct rb_root *root);

#endif