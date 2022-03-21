#include <apps.h>
#include <Kernel/ink.h>
#include <Kernel/scheduler/thread.h>

#include "./dijkstra.h"

#include <msp430.h>
#include <stdint.h>
#include <stdio.h>

#ifndef RST_TIME
#define RST_TIME 25000
#endif


#define N_NODES   25
#define Q_SIZE    4 * N_NODES

#define INFINITY  0xFFFF
#define UNDEFINED 0xFFFF

typedef struct {
	uint16_t dist;
	uint16_t prev;
} node_t;

typedef struct {
	uint16_t node;
	uint16_t dist;
	uint16_t prev;
} queue_t;

/**
 * 1. TEB declaration here.
 */
TEB(task_init);
TEB(task_init_list);
TEB(task_select_nearest_node);
TEB(task_find_shorter_path);
TEB(task_done);

/**
 * 2. Shared variable declaration here. (714bytes)
 */
__shared(
#if (OUR2==0)   //1~7
    queue_t queue[Q_SIZE]; 
    uint16_t deq_idx;
    uint16_t enq_idx;
    uint16_t node_idx;
    uint16_t src_node;
    queue_t nearest_node;
    node_t node_list[N_NODES];
#else           //1-5-2-3-4-6-7
    queue_t queue[Q_SIZE];
    uint16_t src_node;
    uint16_t deq_idx;
    uint16_t enq_idx;
    uint16_t node_idx;
    queue_t nearest_node;
    node_t node_list[N_NODES];
#endif
)

TEB(task_init)//0
{
    __SET(deq_idx) = 0;
    __SET(enq_idx) = 0;

    // Enqueue.
	__SET(queue[0].node) = __GET(src_node);
	__SET(queue[0].dist) = 0;
	__SET(queue[0].prev) = UNDEFINED;
	++__SET(enq_idx);

	NEXT(1);
}


TEB(task_init_list)//1
{
    uint16_t i, sn;

    for (i = 0; i < N_NODES; i++) {
    	__SET(node_list[i].dist) = INFINITY;
    	__SET(node_list[i].prev) = UNDEFINED;
    }

    sn = __GET(src_node);
    __SET(node_list[sn].dist) = 0;
    __SET(node_list[sn].prev) = UNDEFINED;

    sn++;
    // Test nodes 0, 1, 2, 3.
    if (sn < 4) {
    	__SET(src_node) = sn;
    } else {
    	__SET(src_node) = 0;
    }

    NEXT(2);
}


TEB(task_select_nearest_node)//2
{
	uint16_t i = __GET(deq_idx);

	if (__GET(enq_idx) != i) {
		
		// Dequeue nearest node.
		__SET(nearest_node.node) = __GET(queue[i].node);
		__SET(nearest_node.dist) = __GET(queue[i].dist);
		__SET(nearest_node.prev) = __GET(queue[i].prev);
		i++;
		if (i < Q_SIZE) {
			__SET(deq_idx) = i;
		} else {
			__SET(deq_idx) = 0;
		}

		__SET(node_idx) = 0;
		NEXT(3);
	} else {
		NEXT(4);
	}
}


TEB(task_find_shorter_path)//3
{
	uint16_t cost, node, dist, nearest_dist, i;

	node = __GET(nearest_node.node);
	i = __GET(node_idx);
	cost = adj_matrix[node][i];

	if (cost != INFINITY) {
		nearest_dist = __GET(nearest_node.dist);
		dist = __GET(node_list[i].dist);
		if (dist == INFINITY || dist > (cost + nearest_dist)) {
			__SET(node_list[i].dist) = nearest_dist + cost;
			__SET(node_list[i].prev) = node;

			// Enqueue.
			uint16_t j = __GET(enq_idx);
			if (j == (__GET(deq_idx) - 1)) {
				//LOG("QUEUE IS FULL!");
			}
		    __SET(queue[j].node) = i;
		    __SET(queue[j].dist) = nearest_dist + cost;
		    __SET(queue[j].prev) = node;
			j++;
			if (j < Q_SIZE) {
				__SET(enq_idx) = j;
			} else {
				__SET(enq_idx) = 0;
			}
		}
	}

	if (++__SET(node_idx) < N_NODES) {
		NEXT(3);
	} else {
		NEXT(2);
	}
}


TEB(task_done)//4
{
    __no_operation();
    NEXT(0);
}

/**
 * 0. Benchmark app Init here.
 */
void _benchmark_dijkstra_init()
{
    TASK(TASK_PRI);

    TEB_INIT(TASK_PRI, task_init,                   3,  may_war_set_dijkstra[0][0], may_war_set_dijkstra[0][1], teb_breaking_dijkstra[0]);       //0
    TEB_INIT(TASK_PRI, task_init_list,              16, may_war_set_dijkstra[1][0], may_war_set_dijkstra[1][1], teb_breaking_dijkstra[1]); //1
    TEB_INIT(TASK_PRI, task_select_nearest_node,    5,  may_war_set_dijkstra[2][0], may_war_set_dijkstra[2][1], teb_breaking_dijkstra[2]); //2
    TEB_INIT(TASK_PRI, task_find_shorter_path,      3,  may_war_set_dijkstra[3][0], may_war_set_dijkstra[3][1], teb_breaking_dijkstra[3]);    //3
    TEB_INIT(TASK_PRI, task_done,                   1,  may_war_set_dijkstra[4][0], may_war_set_dijkstra[4][1], teb_breaking_dijkstra[4]);  //4

    __SIGNAL(1);
}
