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

/**
* sched_job - FCFS 스케쥴링과 SJF 스케쥴링에서 job의 스케쥴링을 수행
* @info: 시간 정보 기록 (response time)
* @now: 현재 시간
* @job: 스케쥴 된job
*
* response time과 turnaround time을 계산
*/
static inline sched_time_t sched_job(struct time_info *info,
                                     const sched_time_t now,
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

/**
* job_arrived - 현재 job이 도착한 상태인지 확인
* @job: job의 정보
* @now: 현재 시간
*/
static inline int job_arrived(const sched_time_t now,
                              const struct job_info *job)
{
        return job->arrived <= now;
}

/**
* get_shortest_job - shortest job을 구함
* @root: 대기 목록 레드블랙트리의 루트
*
* 대기 목록 레드블랙트리의 가장 왼쪽 리프가 shortest job
*/
static inline struct wait_job *get_shortest_job(struct rb_root *root)
{
        return container_of(rb_first(root), struct wait_job, sjf_node);
}

extern void sjf_push_wait_job(struct rb_root *root, const struct job_info *job,
                              const int idx);
extern void sjf_pop_wait_job(struct wait_job *wjob, struct rb_root *root);

#define NR_RR_QUANTUM   4

/**
* get_rr_next - Round Robin 스케쥴링의 다음 스케쥴 될 job을 구함
* @rq: 대기 목록 큐
*
* 대기 목록 큐의 다음 헤드가 이번에 스케쥴 될 job
*/
static inline struct wait_job *get_rr_next(const struct list_head *rq)
{
        return container_of(rq->next, struct wait_job, rr_list);
}

/**
* first_sched - 처음 스케쥴 된 job인지 확인
* @wjob: 스케쥴 된 대기 중이던 job
*/
static inline int first_sched(const struct wait_job *wjob)
{
        return wjob->run_time == 0;
}

/**
* job_done - job이 끝났는지 확인
* @wjob: 스케쥴 된 대기 중이던 job
*/
static inline int job_done(const struct wait_job *wjob)
{
        return wjob->run_time == wjob->job->amount_time;
}

extern sched_time_t rr_sched_job(struct time_info *info,
                                 const sched_time_t now,
                                 struct wait_job *wjob);
extern void rr_push_wait_job(struct list_head *rq, struct job_info *job);
extern void rr_repush_wait_job(struct list_head *rq);
extern void rr_pop_wait_job(struct wait_job *wjob, struct time_info *info,
                            const sched_time_t now);

#endif