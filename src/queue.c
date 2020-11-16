/** @file
 * Implementacja klasy kolejki do przeszukiwania planszy metodą BFS.
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */
 
#include <stdlib.h>
#include <stdint.h>
#include "queue.h"

bool empty_queue(queue_t queue) {
	return (queue.start == NULL);
}

queue_t init_queue() {
	queue_t queue;
	queue.start = NULL;
	return queue;
}

field_t front_of_queue(queue_t queue) {
	return queue.start->field;
}

bool enqueue(queue_t *queue, field_t field) {
	if(queue->start == NULL) {
		queue->start = (list_t *)malloc(sizeof(list_t));
		queue->start->field = field;
		queue->end = queue->start;
	}
	else {
		list_t *temp;
		temp = (list_t *)malloc(sizeof(list_t));
		temp->next = NULL;
		temp->field = field;
		queue->end->next = temp;
		queue->end = queue->end->next;
	}
	return true;
}

field_t dequeue(queue_t *queue) {
	list_t *temp;
	field_t field;
	temp = queue->start;
	field = queue->start->field;
	if(queue->start == queue->end) {
		queue->start = NULL;
	}
	else {
		queue->start = queue->start->next;
	}
	free(temp);
	return field;
}

void clear_queue(queue_t *queue) {
	while(!empty_queue(*queue)) {
		dequeue(queue);
	}
}
