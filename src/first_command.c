/** @file
 * Implementacja funkcji zajmujących się czytaniem linijki z pierwszym poleceniem.
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "auxiliary_functions.h"
#include "gamma.h"
#include "batch_mode.h"

#define BASIC_ARRAY_LENGTH 10 ///< ustalamy bazową długość tablicy na polecenie

static command_t parse_command(char *command, size_t length, bool *correct_command);


/** @brief Konwertuje linijkę z wejścia.
 * Funkcja przepisuje daną linijkę (o ile jest prawidłowa)
 * do struktury @ref command_t. Linijka jest prawidłowa jeśli:
 * a) zawiera dokładnie 4 wartości liczbowe,
 * b) nieprzekraczające zakresu typu uint32_t.
 * @param[in] command 			- wskaźnik na pamięć zawierającą niepustą
 * 								linijkę z poleceniem,
 * @param[in] length 			- długość linijki zawierającej polecenie,
 * @param[out] correct_command 	- po zakończeniu wykonania funkcji będzie zawierać
 * 								informację czy podana linijka była prawidłowa i
 * 								czy została skonwertowana.
 * @return Struktura zawierająca polecenie do wykonania. Jeśli linijka nie
 * zawierała dokładnie 4 liczb - argumentów potrzebnych do utworzenia planszy -
 * zwracana struktura również jest nieprawidłowa.
 */
command_t parse_command(char *command, size_t length, bool *correct_command) {
	command_t parsed_command;
	parsed_command.name = command[0];
	char *endptr = &command[1];
	int i = 0;
	while(i < MAX_COMMAND_ARGS && *endptr != command[length - 1]) {
		unsigned long value = strtoul(endptr, &endptr, 10);
		if(value > UINT32_MAX || errno == ERANGE) {
			errno = 0;
			break;
		}
		parsed_command.args[i] = (uint32_t)value;
		i++;
	}
	parsed_command.args_length = i;
	if(*endptr == command[length - 1] && i == MAX_COMMAND_ARGS) {
		*correct_command = true;
	}
	return parsed_command;
}

command_t read_first_command(int *counter, bool *eof) {
	bool error = false;
	size_t command_length = 0;
	char *command;
	bool correct_command = false;
	command_t parsed_command = init_command();
	while(!(*eof) && !correct_command && errno != ENOMEM) {
		command = read_one_line(&command_length, eof, &error);
		(*counter)++;
		if(error) {
			fprintf(stderr, "ERROR %d\n", *counter);
		}
		else if(command[0] != '\0' && command[0] != '#' && !(*eof)) {
			parsed_command = parse_command(command, command_length, &correct_command);
			if(!correct_command) {
				fprintf(stderr, "ERROR %d\n", *counter);
			}
		}
		free(command);
	}
	return parsed_command;
}
