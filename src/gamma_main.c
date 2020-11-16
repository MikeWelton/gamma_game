/** @file
 * Plik zawierający funkcję main programu.
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#include <stdio.h>
#include <errno.h>
#include "gamma.h"
#include "first_command.h"
#include "batch_mode.h"
#include "interactive_mode.h"
#include "auxiliary_functions.h"


/** @brief Główna funkcja programu.
 * Na początku funkcja próbuje uzyskać pierwsze polecenie zadające dalszy tryb
 * działania. Następnie, jeśli polecenie zostało uzyskane i nie otrzymano
 * komunikatu o błędach, funkcja próbuje utworzyć nową grę. Jeśli się to udało
 * funkcja przechodzi do odpowiedniego trybu rozgrywki. W przeciwnym przypadku,
 * próbuje uzyskać polecenie jeszcze raz. W sytuacji, gdy pojawia się komunikat
 * o błędzie krytycznym, funkcja kończy działanie z kodem 1.
 * @return Wartość 0, jeśli program zakończył działanie pomyślnie lub
 * wartość 1, jeśli wystąpiły błędy (z pamięcią lub żądana plansza w interactive
 * mode była zbyt duża).
 */
int main(void) {
	bool board_created = false,
	eof = false;
	int counter = 0,
	exit_code = 0;
	command_t first_command;
	gamma_t *g = NULL;
	while(!board_created && !eof) {
		first_command = read_first_command(&counter, &eof);
		if(errno == ENOMEM) {
			return 1;
		}
		if(!eof) {
			uint32_t *args = first_command.args;
			g = gamma_new(args[0], args[1], args[2], args[3]);
			if(g != NULL) {
				board_created = true;
			}
			else {
				fprintf(stderr, "ERROR %d\n", counter);
			}
		}
	}
	if(!eof) {
		if(first_command.name == 'B') {
			fprintf(stdout, "OK %d\n", counter);
			exit_code = batch_mode(g, counter);
		}
		else if(first_command.name == 'I') {
			exit_code = interactive_mode(g);
		}
	}
	gamma_delete(g);
	return exit_code;
}
