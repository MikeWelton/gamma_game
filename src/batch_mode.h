/** @file
 * Interfejs klasy obsługującej tryb wsadowy (batch mode).
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#ifndef BATCH_MODE_H
#define BATCH_MODE_H

#include "gamma.h"


/** @brief Czyta jedną linijkę z wejścia.
 * Czyta po kolei znaki i zapisuje je w tablicy aż do wystąpienia znaku nowej lini
 * lub pojawienia się błędu.
 * Pomija komentarze oraz białe
 * @param[out] command_length 	- wskaźnik, pod który, na koniec działania
 * 								funkcji zostaje zapisana długość zapisanej linijki,
 * 								w przypadku wykrycia błędu, wartość ta może być
 * 								nieprawidłowa,
 * @param[in,out] eof 			- wskaźnik na zmienną zapisującą informację o
 * 								końcu danych wejściowych,
 * @param[in,out] error 		- wskaźnik na zmienną zapisującą informację o
 * 								błędzie w danych wejściowych, dane wejściowe są
 * 								błędne jeśli:
 * 								a) pierwszy znak nie jest poprawną literą, czyli
 * 								poprawną nazwą polecenia w trybie wsadowym,
 * 								b) któryś z kolejnych znaków nie jest liczbą ani
 * 								białym znakiem.
 * @return Linijka z poleceniem. Prawidłowa linijka z poleceniem zwracana przez
 * funkcję zaczyna się od litery, po której następuje biały znak. Potem
 * następują argumenty liczbowe oddzielone od siebie dokładnie jednym białym
 * znakiem przeczytanym z wejścia. Po ostatnim argumencie występuje jeden znak '\0'.
 * Jeśli w czytanych danych wejściowych wykryto błąd lub wystąpił znak EOF,
 * zwracana linijka również nie będzie prawidłowa.
 */
char* read_one_line(size_t *command_length, bool *eof, bool *error);

/** @brief Obsługuje batch mode (tryb wsadowy).
 * Główna funkcja zajmująca się batch mode. W pętli pobiera kolejne linijki z
 * wejścia, konweruje je do struktury @ref command_t, następnie wykonuje zadane
 * w nich polecenia lub wypisuje informacje o błędzie jeśli są nieprawidłowe.
 * Na koniec działania zwalnia pamięć alokowaną na strukturę @ref gamma_t.
 * @param[in,out] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in,out] counter 	- licznik przeczytanych linijek z wejścia.
 * @return Wartość 0, jeśli gra przebiegła prawidłowo lub 1, jeśli wystąpiły błędy
 * np. brak pamięci.
 */
int batch_mode(gamma_t *g, int counter);

#endif /* BATCH_MODE_H */
