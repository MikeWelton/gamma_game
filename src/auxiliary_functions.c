/** @file
 * Implementacja funkcji pomocniczych.
 * Funkcje zajmują się m.in. pamięcią oraz
 * inicjalizacją i usuwaniem tablic oraz operujących na pomocniczej strukturze
 * @ref command_t
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#include <string.h>
#include <ctype.h>
#include "auxiliary_functions.h"
#include "queue.h"


#define BASIC_ARRAY_LENGTH 10 ///< ustalamy bazową długość tablicy na polecenie


void** init_board(size_t size_of_type, uint32_t width, uint32_t height) {
	size_t size_of_ptr = sizeof(void *);
	void **array = calloc((size_t)width, size_of_ptr);
	if(!array) {
		return NULL;
	}
	
	for(uint32_t i = 0; i < width; i++) {
		array[i] = calloc((size_t)height, size_of_type);
		if(!array[i]) {
			delete_board(array, i);
			return NULL;
		}
	}
	
	return array;
}

void delete_board(void **array, uint32_t width) {
	if(array) {
		for(uint32_t i = 0; i < width; i++) {
			free(array[i]);
		}
		free(array);
	}
}

void init_boolean_array(bool *array, uint32_t size) {
	for(uint32_t i = 0; i < size; i++) {
		array[i] = true;
	}
}

int number_of_digits(uint32_t num) {
	int num_of_digits = 1;
	while(num >= 10) {
		num /= 10;
		num_of_digits++;
	}
	return num_of_digits;
}


command_t init_command() {
	command_t command;
	command.name = '\0';
	command.args_length = 0;
	for(int i = 0; i < MAX_COMMAND_ARGS; i++) {
		command.args[i] = 0;
	}
	return command;
}

void* init_array(size_t type_size) {
	void *array = calloc(BASIC_ARRAY_LENGTH, type_size);
	return array;
}

char* realloc_char_array(char *array, size_t *length) {
	size_t old_length = (*length);
	(*length) *= 2;
	array = realloc(array, (*length));
	memset(&(array[old_length]), '\0', old_length);
	return array;
}
