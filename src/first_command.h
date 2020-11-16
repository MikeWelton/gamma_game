/** @file
 * Interfejs pliku zajmującego się czytaniem linijki z pierwszym poleceniem.
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#ifndef FIRST_COMMAND_H
#define FIRST_COMMAND_H

#include <stdbool.h>
#include <stdint.h>
#include "auxiliary_functions.h"

/** @brief Czyta pierwsze polecenie zadające tryb rozgrywki.
 * Funkcja (używając @ref read_one_line udostępnianej
 * przez @ref batch_mode.h) czyta kolejne linijki z wejścia do momentu
 * wystąpienia prawidłowego, polecenia zadającego tryb rozgrywki, znaku końca
 * danych lub wystapienia błędu. Prawidłowe polecenie:
 * a) zaczyna się od znaku I lub B, po którym następuje choć jeden biały znak,
 * b) następnie występują dokładnie 4 argumenty liczbowe, nieprzekraczające
 * zakresu typu uint32_t, oddzielone dowolną liczbą białych znaków,
 * c) zostaje zakończone znakiem końca lini.
 * @param[in,out] counter 	- wskaźnik na zmienną będącą licznikiem linijek, które
 * 							wystąpiły na wejściu,
 * @param[in,out] eof 		- wskaźnik na zmienną przechowującą informację o
 * 							wystąpieniu na wejściu znaku końca danych.
 * @return Struktura @ref command_t, zawierająca linijkę z poleceniem zadającym
 * tryb gry. Polecenie może być nieprawidłowe, jeśli w czasie działanie funkcji,
 * na wejściu wystąpił znak końca danych.
 */
command_t read_first_command(int *counter, bool *eof);

#endif /* FIRST_COMMAND_H */
