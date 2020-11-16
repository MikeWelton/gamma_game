/** @file
 * Interfejs klasy kolejki do przeszukiwania planszy metodą BFS.
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */
 
#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>

/**
 * Pole z planszy.
 */
typedef struct field {
	uint32_t x; 	///< współrzędna x(pozioma, oś odciętych) pola na planszy
	uint32_t y; 	///< współrzędna y(pionowa, oś rzędnych) pola na plnaszy
} field_t;

/**
 * Typowa lista na wskaźnkach.
 */
typedef struct list {
	field_t field; 	///< pole z planszy, zawartość każdego elementu listy
	struct list *next; 	///< wskaźnik na następny element listy
} list_t;

/**
 * Implementacja listowa kolejki.
 */
typedef struct queue {
	list_t *start; 	///< początek kolejki
	list_t *end; 	///< koniec kolejki
} queue_t;

/** @brief Sprawdza czy kolejka jest pusta.
 * @param[in] queue 	- kolejka, która ma zostać sprawdzona.
 * @return Wartość @p true jeśli kolejka jest pusta, wartość @p false w przeciwnym
 * w przeciwnym wypadku.
 */
bool empty_queue(queue_t queue);

/** @brief Inicjalizuje kolejkę.
 * @return Zainicjalizowana kolejka.
 */
queue_t init_queue();

/** @brief Podaje pierwszy element kolejki.
 * @param[in] queue 	- kolejka, której front chcemy otrzymać.
 * @return Pierwszy element kolejki.
 */
field_t front_of_queue(queue_t queue);

/** @brief Wkłada element na koniec kolejki.
 * @param[in] queue 	- wskaźnik na kolejkę, do której chcemy dodać element,
 * @param[in] field 	- pole na planszy, ktore chcemy włożyć na koniec kolejki.
 * @return Wartość @p true, jeśli dodano element do kolejki lub @p false, jeśli
 * tak się nie stało z powodu braku pamięci.
 */
bool enqueue(queue_t *queue, field_t field);

/** @brief Wyjmuje pierwszy element w kolejce.
 * @param[in] queue 	- wskaźnik na kolejkę, z której chcemy otrzymać
 * 						pierwszy element.
 * @return Pierwszy element w kolejce.
 */
field_t dequeue(queue_t *queue);

/** @brief Czyści całą kolejkę.
 * @param[in] queue 	- kolejka, która ma zostać wyczyszczona.
 */
void clear_queue(queue_t *queue);

#endif /* QUEUE_H */
