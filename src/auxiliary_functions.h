/** @file
 * Interfejs funkcji pomocniczych.
 * Zawiera funkcje zajmujące się m.in. pamięcią oraz
 * inicjalizacją i usuwaniem tablic oraz zawierający pomocniczą strukturę
 * @ref command_t
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#ifndef MEMORY_AND_INIT_H
#define MEMORY_AND_INIT_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>


#define MAX_COMMAND_ARGS 4 ///< każde polecenie zawiera max. 4 argumenty liczbowe


/**
 * Struktura przechowująca jedno polecenie z wejścia.
 */
typedef struct command {
	char name; ///< nazwa polecenia, litera ze zbioru {B, I, m, g, b, f, q, p}
	int args_length; ///< liczba argumentów polecenia, liczba od 0 do 4
	uint32_t args[MAX_COMMAND_ARGS]; ///< tablica argumentów liczbowych polecenia
} command_t;


/** @brief Inicjalizuje nową planszę dowolnego typu.
 * @param[in] size_of_type 	- rozmiar typu, którego mają być pola planszy,
 * @param[in] width 		- szerokość planszy,
 * @param[in] height 		- wysokość planszy.
 * @return Zainicjalizowana plansza lub NULL, jeśli zabrakło pamięci.
 */
void** init_board(size_t size_of_type, uint32_t width, uint32_t height);

/** @brief Usuwa planszę.
 * @param[in,out] array - usuwana plansza,
 * @param[in] width 	- szerokość planszy.
 */
void delete_board(void **array, uint32_t width);

/** @brief Inicjalizuje tablice typu bool na wartość true.
 * @param[in,out] array - tablica, która ma zostać zainicjalizowana,
 * @param[in] size 		- rozmiar tablicy.
 */
void init_boolean_array(bool *array, uint32_t size);

/** @brief Liczy ilość cyfr liczby.
 * @param[in] num - liczba, której ilość cyfr chcemy policzyć.
 * @return Ilość cyfr liczy, wartość od 0 do 10 (@p UINT_LIMIT).
 */
int number_of_digits(uint32_t num);

/** @brief Inicjalizuje strukturę command_t.
 * @return Zainicjalizowana struktura.
 */
command_t init_command();

/** @brief Inicjalizuje tablicę dowolnego typu.
 * Alokuje pamięć na tablicę o bazowej długości i wypełnia ją zerami.
 * @param[in] type_size 	- rozmiar typu, którego ma być tablica.
 * @return Zainicjalizowana tablica.
 */
void* init_array(size_t type_size);

/** @brief Realokuje tablicę znaków.
 * @param[in,out] array 	- tablica, której pamięć ma być realokowana,
 * @param[in,out] length 	- obecna długość tablicy, zwiększana dwukrotnie
 * 							podczas realokacji pamięci.
 * @return Wskaźnik na pamięć, w której znajduje się realokowana tablica znaków.
 */
char* realloc_char_array(char *array, size_t *length);

#endif /* MEMORY_AND_INIT_H */
