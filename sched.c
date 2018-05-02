#include <stdlib.h>
#include "sched.h"

struct time_info get_fcfs_time(const struct job_head *head)
{
        struct job_info *jobs = head->jobs;
        const int jcnt = head->job_cnt;
        struct time_info info = {
                .tard_time = 0,
                .resp_time = 0
        };
        sched_time_t now = jobs->arrived;

        for (int i = 0; i < jcnt; i++) {
                if ((jobs + i)->arrived > now)
                    now = (jobs + i)->arrived;
                now += sched_job(&info, now, jobs + i);
        }

        return info;
}

void sjf_push_wait_job(struct rb_root *root, const struct job_info *job,
                       const int idx)
{
        struct wait_job *new = malloc(sizeof(struct wait_job));
        struct rb_node **node = &(root->rb_node), *parent = NULL;

        new->job = job;
        new->idx = idx;
        while (*node) {
                struct wait_job *this = container_of(*node, struct wait_job,
                                                     sjf_node);
                parent = *node;
                if (job->amount_time < this->job->amount_time)
                        node = &((*node)->rb_left);
                else if (job->amount_time > this->job->amount_time)
                        node = &((*node)->rb_right);
                else if (job->arrived < this->job->arrived)
                        node = &((*node)->rb_left);
                else
                        node = &((*node)->rb_right);
        }

        rb_link_node(&new->sjf_node, parent, node);
        rb_insert_color(&new->sjf_node, root);
}

void sjf_pop_wait_job(struct wait_job *wjob, struct rb_root *root)
{
        rb_erase(&wjob->sjf_node, root);
        free(wjob);
}

#define job_arrived(now, job)        ((job)->arrived <= (now))
#define sjf_pull_arrived_jobs(trav, job_cnt, now, jobs, wait_tree)            \
        do {                                                                  \
                for (int i = (trav); i < (job_cnt); i++) {                    \
                        if (job_arrived((now), (jobs) + i)) {                 \
                                sjf_push_wait_job((wait_tree),                \
                                                  (jobs) + i, i);             \
                                (trav)++;                                     \
                        } else {                                              \
                                break;                                        \
                        }                                                     \
                }                                                             \
        } while (0)

struct time_info get_sjf_time(const struct job_head *head)
{
        struct job_info *jobs = head->jobs;
        const int jcnt = head->job_cnt;
        struct time_info info = {
                .tard_time = 0,
                .resp_time = 0
        };
        sched_time_t now = jobs->arrived;

        struct rb_root wait_tree = RB_ROOT;
        struct wait_job *next_wjob;
        int trav = 0;

        do {
                sjf_pull_arrived_jobs(trav, jcnt, now, jobs, &wait_tree);
                if (RB_EMPTY_ROOT(&wait_tree)) {
                        now = (jobs + trav)->arrived;
                        continue;
                }
                next_wjob = get_shortest_job(&wait_tree);
                now += sched_job(&info, now, next_wjob->job);
                sjf_pop_wait_job(next_wjob, &wait_tree);
        } while (!RB_EMPTY_ROOT(&wait_tree) || trav < jcnt);

        return info;
}

#define first_sched(wjob)   ((wjob)->run_time == 0)
#define job_done(wjob)  ((wjob)->run_time == (wjob)->job->amount_time)

sched_time_t rr_sched_job(struct time_info *info, const int now,
                          struct wait_job *wjob)
{
        sched_time_t rest_time = wjob->job->amount_time - wjob->run_time;
        sched_time_t perf_time = min(rest_time, NR_RR_QUANTUM);

        if (first_sched(wjob))
                info->resp_time += now - wjob->job->arrived;

        wjob->run_time += perf_time;
        return perf_time;
}

void rr_push_wait_job(struct list_head *rq, struct job_info *job)
{
        struct wait_job *new = malloc(sizeof(struct wait_job));

        new->job = job;
        new->run_time = 0;
        list_add_tail(&new->rr_list, rq);
}

void rr_repush_wait_job(struct list_head *rq)
{
        list_rotate_left(rq);
}

void rr_pop_wait_job(struct wait_job *wjob, struct time_info *info,
                     const int now)
{
        list_del(&wjob->rr_list);
        info->tard_time += now - wjob->job->arrived;
        free(wjob);
}

struct time_info get_rr_time(const struct job_head *head)
{
        struct job_info *jobs = head->jobs;
        const int jcnt = head->job_cnt;
        struct time_info info = {
                .tard_time = 0,
                .resp_time = 0
        };

        LIST_HEAD(rr_queue);
        struct wait_job *next_wjob;
        sched_time_t now = jobs->arrived;
        int trav = 0;

        do {
                if (list_empty(&rr_queue)) {
                        now = (jobs + trav)->arrived;
                        for (int i = trav; i < jcnt; i++) {
                                if (job_arrived(now, jobs + i)) {
                                        rr_push_wait_job(&rr_queue, jobs + i);
                                        trav++;
                                } else
                                        break;
                        }
                        continue;
                }

                next_wjob = get_rr_next(&rr_queue);
                now += rr_sched_job(&info, now, next_wjob);

                for (int i = trav; i < jcnt; i++) {
                        if (job_arrived(now, jobs + i)) {
                                rr_push_wait_job(&rr_queue, jobs + i);
                                trav++;
                        } else
                                break;
                }

                if (job_done(next_wjob))
                        rr_pop_wait_job(next_wjob,&info, now);
                else
                        rr_repush_wait_job(&rr_queue);
        } while (!list_empty(&rr_queue) || trav < jcnt);

        return info;
}
