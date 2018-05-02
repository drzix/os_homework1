#include <stdlib.h>
#include "sched.h"

/**
* get_fcfs_time - First Come First Served 스케쥴링으로 수행된 job들의
*                 총 turnaround time과 총 response time을 구함
* @head: job 목록
*
* 모든 job을 차례대로 순회하며 시간을 누적하여 계산
*/
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

/**
* sjf_push_wait_job - 새로 도착한 job을 대기 목록에 넣음
* @root: 대기 목록 레드블랙트리의 루트
* @job: 새로 도착한 job
* @idx: job의 job 목록에서의 인덱스
*
* 새로 도착한 job을 amount time과 인덱스 순의 비교로 레드블랙트리에 삽입
*/
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

/**
* sjf_pop_wait_job - 스케쥴 된 대기 중이던 job을 대기 목록에서 삭제
* @wjob: 스케쥴 된 대기 중이던 job
* @root: 대기 목록 레드블랙트리의 루트
*
* 레드블랙트리에서 노드를 삭제
*/
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

/**
* get_sjf_time - Shortest Job First 스케쥴링으로 수행된 job들의
*                총 turnaround time과 총 response time을 구함
* @head: job 목록
*
* 모든 job을 차례대로 순회하며 시간을 누적하면서
* 어떤 job이 끝난 시점을 포함한 이전 시간에 도착한
* 도착한 job들을 레드블랙트리로 관리
*/
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

/**
* rr_sched_job - Round Robin 스케쥴링 방식으로 job의 스케쥴링을 수행
* @info: 시간 정보 기록 (response time)
* @now: 현재 시간
* @wjob: 스케쥴 된 대기 중이던 job
*
* 처음 스케쥴 된 경우 response time을 계산
* job의 남은 시간과 퀀텀 중에서 더 작은 시간만 수행
*/
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

/**
* rr_push_wait_job - 새로 도착한 job을 대기 목록 큐에 넣음
* @rq: 대기 목록 큐
* @job: 새로 도착한 job
*
* 새로 도착한 job을 리스트의 tail에 삽입
*/
void rr_push_wait_job(struct list_head *rq, struct job_info *job)
{
        struct wait_job *new = malloc(sizeof(struct wait_job));

        new->job = job;
        new->run_time = 0;
        list_add_tail(&new->rr_list, rq);
}

/**
* rr_repush_wait_job - 아직 끝나지 않은 job을 다시 대기 큐에 넣음
* @rq: 대기 목록 큐
*
* 현재 head인 노드를 tail로 바꿔준다
* 반드시 하나 이상의 job이 대기 상태에 있어야 함
*/
void rr_repush_wait_job(struct list_head *rq)
{
        list_rotate_left(rq);
}

/**
* rr_pop_wait_job - 스케쥴 된 대기 중이던 job을 대기 목록에서 삭제
* @wjob: 스케쥴 된 대기 중이던 job
* @info: 시간 정보 기록 (turnaround time)
* @now: 현재 시간
*
* 대기 목록 큐에서 job을 삭제 및 turnaround time 계산
*/
void rr_pop_wait_job(struct wait_job *wjob, struct time_info *info,
                     const int now)
{
        list_del(&wjob->rr_list);
        info->tard_time += now - wjob->job->arrived;
        free(wjob);
}

/**
* get_rr_time - Round Robin 스케쥴링으로 수행된 job들의
*               총 turnaround time과 총 response time을 구함
* @head: job 목록
*
* 기본적인 Round Robin 스케쥴 방식의 알고리즘에 의거
*/
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
                        rr_pop_wait_job(next_wjob, &info, now);
                else
                        rr_repush_wait_job(&rr_queue);
        } while (!list_empty(&rr_queue) || trav < jcnt);

        return info;
}