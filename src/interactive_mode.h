/** @file
 * Interfejs klasy obsługującej tryb interaktywny (interactive mode).
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#ifndef INTERACTIVE_MODE_H
#define INTERACTIVE_MODE_H

#include "gamma.h"

/** @brief Obsługuje interactive mode (tryb interaktywny).
 * Główna funkcja zarządzająca działaniem w trybie interaktywnym. Wywołuje w
 * pętli funkcję przeprowadzającą jedną turę gry. Na koniec działania wypisuje
 * podsumowanie gry i zwalnia pamięć alokowaną na @ref gamma_t.
 * @param[in,out] g 	- wskaźnik na strukturę przechowującą stan gry.
 * @return Wartość 0, jeśli rozgrywka w trybie interaktywnym zakończyła się w
 * sposób prawidłowy lub 1, gdy nie utworzono rozgrywki ze względu na zbyt dużą
 * plansze.
 */
int interactive_mode(gamma_t *g);

#endif /* INTERACTIVE_MODE_H */
