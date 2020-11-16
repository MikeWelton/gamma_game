/** @file
 * Implementacja klasy obsługującej tryb wsadowy (batch mode).
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "batch_mode.h"
#include "auxiliary_functions.h"


#define BASIC_ARRAY_LENGTH 10 ///< Ustalamy bazową długość tablicy na polecenie


bool batch_mode_active = false; /**< Zmienna globalna posiadająca informację czy
 * tryb wsadowy (batch mode) jest uruchomiony. Jest konieczna ponieważ funkcja
 * @ref read_one_line może być wywołana przed uruchomieniem batch mode.
 */


static command_t parse_command(char *command, size_t length, bool *correct_command);

static bool check_number_of_args(command_t command);

static void select_function_to_call_and_print_answer(command_t command, gamma_t *g);

static void manage_gamma_board_call(gamma_t *g);

static uint64_t call_function(command_t command, gamma_t *g);

static bool legal_name_in_batch(char c);

static bool choose_condition_depending_on_mode(int c);

static void check_char(int c, size_t *i, char **command, size_t *array_length,
					   bool *prev_was_white_char, bool *error);


/** @brief Konwertuje linijkę z wejścia.
 * Funkcja przepisuje daną linijkę (o ile jest prawidłowa)
 * do struktury @ref command_t. Linijka jest prawidłowa jeśli:
 * a) zawiera nie więcej niż 3 wartości liczbowe (@ref MAX_COMMAND_ARGS - 1),
 * b) nieprzekraczające zakresu typu uint32_t.
 * @param[in] command 			- wskaźnik na pamięć zawierającą niepustą
 * 								linijkę z poleceniem,
 * @param[in] length 			- długość linijki zawierającej polecenie,
 * @param[out] correct_command 	- po zakończeniu wykonania funkcji będzie zawierać
 * 								informację czy podana linijka była prawidłowa i
 * 								czy została skonwertowana.
 * @return Struktura zawierająca polecenie do wykonania. Jeśli parsowana linijka
 * została uznana za nieprawidłową - zwracana struktura również jest nieprawidłowa.
 */
command_t parse_command(char *command, size_t length, bool *correct_command) {
	command_t parsed_command;
	parsed_command.name = command[0];
	char *endptr = &command[1];
	int i = 0;
	while(i < MAX_COMMAND_ARGS - 1 && *endptr != command[length - 1]) {
		unsigned long value = strtoul(endptr, &endptr, 10);
		if(value > UINT32_MAX || errno == ERANGE) {
			errno = 0;
			break;
		}
		parsed_command.args[i] = (uint32_t)value;
		i++;
	}
	parsed_command.args_length = i;
	if(*endptr == command[length - 1]) {
		*correct_command = true;
	}
	return parsed_command;
}

/** @brief Sprawdza poprawność liczby argumentów dla poszczególnych poleceń.
 * @param[in] command 			- struktura zawierająca linijkę z poleceniem.
 * @return Wartość @p true, jeśli liczba argumentów dla danego polecenia jest
 * poprawna lub @p false w przeciwnym wypadku.
 */
bool check_number_of_args(command_t command) {
	switch(command.name) {
		case 'm':
		case 'g':
			if(command.args_length == 3) {
				return true;
			}
			break;
		case 'b':
		case 'f':
		case 'q':
			if(command.args_length == 1) {
				return true;
			}
			break;
		case 'p':
			if(command.args_length == 0) {
				return true;
			}
			break;
	}
	return false;
}

/** @brief Wywołuje odpowiednią funkcję dla danego polecenia.
 * @param[in] command 			- struktura zawierająca linijkę z poleceniem,
 * @param[in] g 				- wskaźnik na strukturę przechowującą stan gry,
 */
void select_function_to_call_and_print_answer(command_t command, gamma_t *g) {
	if(command.name == 'p') {
		manage_gamma_board_call(g);
	}
	else {
		uint64_t answer = call_function(command, g);
		fprintf(stdout, "%lu\n", answer);
	}
}

/** @brief Obsługuje polecenie p wywołujące @ref gamma_board.
 * @param[in] g 			 	- wskaźnik na strukturę przechowującą stan gry,
 */
void manage_gamma_board_call(gamma_t *g) {
	char *board = gamma_board(g);
	if(board == NULL) {
		fprintf(stdout, "0\n");
	}
	else {
		printf("%s", board);
		free(board);
	}
}

/** @brief Wywołuje odpowiednią funkcję, zadaną poleceniem z wejścia.
 * Wywołuje jedną z pięciu funkcji z modułu silnika gry, w zależności od
 * wartości pola name w strukturze @ref command_t.
 * @param[in] command 			- struktura zawierająca linijkę z poleceniem,
 * @param[in] g 				- wskaźnik na strukturę przechowującą stan gry.
 * @return Wartość zwrócona przez wywołaną funkcję z modułu silnika gry.
 */
uint64_t call_function(command_t command, gamma_t *g) {
	uint32_t *args = command.args;
	switch(command.name) {
		case 'm':
			return gamma_move(g, args[0], args[1], args[2]);
		case 'g':
			return gamma_golden_move(g, args[0], args[1], args[2]);
		case 'b':
			return gamma_busy_fields(g, args[0]);
		case 'f':
			return gamma_free_fields(g, args[0]);
		case 'q':
			return gamma_golden_possible(g, args[0]);
	}
	return 0;
}

/** @brief Sprawdza czy podany znak jest legalną nazwą polecenia.
 * @param[in] c 		- sprawdzany znak.
 * @return Wartość @p true, jeśli sprawdzany znak jest legalną nazwą polecenia w
 * trybie wsadowym lub @p false w przeciwnym przypadku.
 */
bool legal_name_in_batch(char c) {
	if (c == 'm' || c == 'g' || c == 'b' || c == 'f' || c == 'q' || c == 'p') {
		return true;
	} else {
		return false;
	}
}

/** @brief Wybiera warunek do sprawdzenia w zależności od trybu.
 *
 * @param[in] c 	- znak, który chcemy sprawdzić pod kątem spełniania warunku.
 * @return Wartość @p true, jeśli znak spełnia wybrany warunek lub @p false
 * w przeciwnym wypadku.
 */
bool choose_condition_depending_on_mode(int c) {
	if(batch_mode_active) {
		return legal_name_in_batch((char)c);
	}
	else {
		return (c == 'B' || c == 'I');
	}
}

/** @brief Sprawdza znak z wejścia i wpisuje do tablicy, jeśli jest prawidłowy.
 *
 * @param[in] c 				- sprawdzany znak,
 * @param[in,out] i				- wskaźnik na licznik znaków w tablicy,
 * @param[in,out] command		- wskaźnik na tablicę, do której wpisujemy polecenie
 * 								z wejścia,
 * @param[in,out] array_length	- wskaźnik na długość tablicy z poleceniem,
 * @param[in,out] prev_was_white_char - wskaźnik na zmienną zapisującą informację
 * 								czy poprzedni znak z wejścia był biały,
 * @param[out] error			- wskaźnik zapisujący informację czy wczytywana
 * 								linijka z poleceniem jest poprawna lub czy
 * 								wystąpił błąd podczas jej wczytywania.
 */
void check_char(int c, size_t *i, char **command, size_t *array_length,
				bool *prev_was_white_char, bool *error) {
	bool condition = choose_condition_depending_on_mode(c);
	if((*i == 0 && condition)
	   || ((*i == 1) && (isspace(c)))
	   || ((*i > 1) && (isdigit(c) || isspace(c)))) {
		if(!isspace(c) || !(*prev_was_white_char)) {
			(*command)[*i] = (char)c;
			(*i)++;
		}
		if(*i >= *array_length) {
			*command = realloc_char_array(*command, array_length);
			if(!(*command)) {
				*error = true;
				errno = ENOMEM;
			}
		}
		if(isspace(c) && !(*prev_was_white_char)) {
			*prev_was_white_char = true;
		}
		else if(!isspace(c) && *prev_was_white_char) {
			*prev_was_white_char = false;
		}
	}
	else {
		*error = true;
	}
}


char* read_one_line(size_t *command_length, bool *eof, bool *error) {
	bool prev_was_white_char = false,
			comment = false,
			newline = false;
	char *command = init_array(sizeof(char));
	if(!command) {
		errno = ENOMEM;
		*error = true;
		return NULL;
	}
	size_t array_length = BASIC_ARRAY_LENGTH;
	size_t i = 0;
	int c;
	*error = false;
	while(!newline && !(*eof) && errno != ENOMEM) {
		c = getchar();
		if(c == EOF) {
			if(i != 0) {
				*error = true;
			}
			*eof = true;
		}
		else if(c == '\n') {
			newline = true;
		}
		else if(c == '#' && i == 0) {
			comment = true;
		}
		else if(!comment && !(*error)) {
			check_char(c, &i, &command, &array_length, &prev_was_white_char, error);
		}
	}
	
	if(prev_was_white_char) {
		--i;
	}
	command[i] = '\0';
	*command_length = i + 1;
	return command;
}

int batch_mode(gamma_t *g, int counter) {
	batch_mode_active = true;
	bool error = false, eof = false;
	size_t command_length = 0;
	bool correct_command;
	command_t parsed_command;
	while(!eof && errno != ENOMEM) {
		char *command = read_one_line(&command_length, &eof, &error);
		counter++;
		if(error) {
			fprintf(stderr, "ERROR %d\n", counter);
		}
		else if(command[0] != '\0' && command[0] != '#' && !eof) {
			correct_command = false;
			parsed_command = parse_command(command, command_length, &correct_command);
			bool correct_num_of_args = check_number_of_args(parsed_command);
			if(correct_command && correct_num_of_args) {
				select_function_to_call_and_print_answer(parsed_command, g);
			}
			else {
				fprintf(stderr, "ERROR %d\n", counter);
			}
		}
		free(command);
	}
	return (errno == ENOMEM);
}

