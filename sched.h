#ifndef _SCHED_H
#define _SCHED_H

#include "rbtree.h"
#include "list.h"

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

static inline sched_time_t sched_job(struct time_info *info,
                                     const int now,
                                     const struct job_info *job)
{
        sched_time_t resp_time = now - job->arrived;

        info->resp_time += resp_time;
        info->tard_time += resp_time + job->amount_time;

        return job->amount_time;
}

struct wait_job {
        const struct job_info           *job;

        struct rb_node                  sjf_node;
        int                             idx;

        struct list_head                rr_list;
        sched_time_t                    run_time;
};

static inline struct wait_job *get_shortest_job(struct rb_root *root)
{
        return container_of(rb_first(root), struct wait_job, sjf_node);
}

extern void sjf_push_wait_job(struct rb_root *root, const struct job_info *job,
                              const int idx);
extern void sjf_pop_wait_job(struct wait_job *wjob, struct rb_root *root);

#define NR_RR_QUANTUM   4

static inline struct wait_job *get_rr_next(struct list_head *rq)
{
        return container_of(rq->next, struct wait_job, rr_list);
}

extern sched_time_t rr_sched_job(struct time_info *info, const int now,
                                 struct wait_job *wjob);
extern void rr_push_wait_job(struct list_head *rq, struct job_info *job);
extern void rr_repush_wait_job(struct list_head *rq);
extern void rr_pop_wait_job(struct wait_job *wjob, struct time_info *info,
                            const int now);

#endif