#include <stdlib.h>
#include "sched.h"

struct time_info get_fcfs_time(const struct job_head *head)
{
        const struct job_info *jobs = head->jobs;
        const int jcnt = head->job_cnt;
        struct time_info info = {
                .tard_time = 0,
                .resp_time = 0
        };
        sched_time_t now = jobs->arrived;

        for (int i = 0; i < jcnt; i++) {
                sched_job(&info, now, jobs + i);
                now += (jobs + i)->amount_time;
        }

        return info;
}

void push_wait_job(struct rb_root *root, const struct job_info *job,
                   const int idx)
{
        struct wait_job *new = malloc(sizeof(struct wait_job));
        struct rb_node **node = &(root->rb_node), *parent = NULL;

        new->job = job;
        new->idx = idx;
        while (*node) {
                struct wait_job *this = container_of(*node, struct wait_job,
                                                     wait_node);
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

        rb_link_node(&new->wait_node, parent, node);
        rb_insert_color(&new->wait_node, root);
}

void pop_wait_job(struct wait_job *wjob, struct rb_root *root)
{
        rb_erase(&wjob->wait_node, root);
        free(wjob);
}

#define job_arrived(now, job)        ((job)->arrived <= (now))
#define pull_arrived_jobs(start, job_cnt, now, jobs, wait_tree)               \
        do {                                                                  \
                for (int i = (start); i < (job_cnt); i++) {                   \
                        if (job_arrived((now), (jobs) + i)) {                 \
                                push_wait_job((wait_tree), (jobs) + i, i);    \
                                (start)++;                                    \
                        } else {                                              \
                                break;                                        \
                        }                                                     \
                }                                                             \
        } while (0)

struct time_info get_sjf_time(const struct job_head *head)
{
        const struct job_info *jobs = head->jobs;
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
                pull_arrived_jobs(trav, jcnt, now, jobs, &wait_tree);
                if (RB_EMPTY_ROOT(&wait_tree)) {
                        now = (jobs + trav)->arrived;
                        continue;
                }
                next_wjob = get_shortest_job(&wait_tree);
                sched_job(&info, now, next_wjob->job);
                now += next_wjob->job->amount_time;
                pop_wait_job(next_wjob, &wait_tree);
        } while (!RB_EMPTY_ROOT(&wait_tree) || trav < jcnt);

        return info;
}

struct time_info get_rr_time(const struct job_head *head)
{
        const struct job_info *jobs = head->jobs;
        const int jcnt = head->job_cnt;
        struct time_info info = {
                .tard_time = 0,
                .resp_time = 0
        };
        return info;
}
