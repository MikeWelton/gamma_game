/** @file
 * Implementacja klasy przechowującej stan gry gamma.
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "gamma.h"
#include "queue.h"
#include "auxiliary_functions.h"


#define UINT_LIMIT 10 ///< Limit długości uint32_t to 10 cyfr (2^32 = 4,294,967,296)

#define BOARD_SIZE_HARD_LIMIT UINT32_MAX /**< Ustalam limit na łączną liczbę pól
 * planszy na 2*32 - 1 = UIN32_MAX.
 */

/**
 * Implementacja struktury przechowującej stan gry.
 */
struct gamma {
	uint32_t **board; 		/**< plansza, zawiera numery graczy, do których
 * 							należą poszczególne pola planszy, bazowo
 * 							wypelniona zerami */
	uint64_t *busy_fields; 	/**< tablica zawierająca ilość pól zajętych
 * 							przez każdego gracza */
	uint64_t *free_fields; 	/**< tablica zawierająca ilość pól, na które
 * 							każdy z graczy może postawić pionek w następnym
 * 							normalnym(nie złotym) ruchu */
	uint32_t *areas; 		/**< tablica zawierająca ilość obaszarow
 * 							zajętych przez każdego z graczy */
	bool *golden_move_available; /**< tablica zawierająca informację czy
 * 							dany gracz wykorzystał już złoty ruch */
	uint64_t all_free_fields; /**< liczba wszystkich wolnych pól na planszy */
	uint32_t players; 		/**< liczba graczy */
	uint32_t max_areas; 	/**< maksymalna liczba obszarów należących do
 * 							jednego gracza */
	uint32_t width; 		/**< szerokość planszy */
	uint32_t height;		/**< wysokość planszy */
	
};


/** @name Funkcje zajmujące się polami
 * Funkcje sprawdzające sąsiądów pola, dostępność zajętych pól i liczące ilość
 * wolnych pól.
 */
///@{
static bool busy_fields_available(uint64_t *busy_fields, uint32_t num_of_players,
                                    uint32_t player);

static bool player_fields_around(gamma_t *g, uint32_t player, uint32_t x,
									uint32_t y);

static uint64_t count_free_fields(gamma_t *g, uint32_t player);
///@}


/** @name Funkcje wykorzystywane przy liczeniu ilości sąsiednich obszarów
 * Grupa funkcji używanych w @ref gamma_move i @ref gamma_golden_move do
 * wyznaczenia liczby obszarów z jakimi sąsiaduje dane pole.
 */
///@{
static void add_to_queue(gamma_t *g, bool **visited, queue_t *queue,
			field_t *field, uint32_t player, uint32_t *coordinate, int factor);

static void add_to_queue_fields_around(gamma_t *g, bool **visited,
								queue_t *queue, field_t field, uint32_t player);

static void make_BFS(gamma_t *g, uint32_t player, bool **visited,
                     field_t field);

static bool is_another_area(gamma_t *g, bool **visited, field_t *field,
                            uint32_t player, uint32_t *coordinate, int factor);

static int number_of_connected_areas(gamma_t *g, uint32_t player, uint32_t x,
                                     uint32_t y);

static int number_of_split_areas(gamma_t *g, uint32_t owner, uint32_t x,
                                 uint32_t y);
///@}


/** @name Funkcje używane przy tworzeniu bufora ze stanem planszy
 * Używane w @ref gamma_board.
 */
///@{
static char* board_for_multiple_digits_players(gamma_t *g, size_t max_num_of_digits);

static char* standard_board(gamma_t *g);
///@}


/** @brief Sprawdza czy isnieją pola zejęte przez innych garczy.
* Funkcja pomocnicza dla funkcji @ref gamma_golden_possible, sprawdzająca czy
		* istnieje choć jedno pole zajęte przez innego gracza.
* @param[in] busy_fields 	 - zawiera informacje ile pól zajęli poszczególni gracze,
* @param[in] num_of_players  - łączna liczba graczy,
* @param[in] player 		 - numer gracza, na rzecz którego dokonywane jest sprawdzenie.
* @return Wartość @p true, jeśli istnieje choć jedno pole zajęte przez innego
* gracza lub @p false, gdy takich pól nie ma.
*/
bool busy_fields_available(uint64_t *busy_fields, uint32_t num_of_players,
                           uint32_t player) {
	for(unsigned int i = 1; i <= num_of_players; i++) {
		if(busy_fields[i] != 0 && i != player) {
			return true;
		}
	}
	return false;
}

/** @brief Sprawdza czy któreś z sąsiadujących pól należy do danego gracza
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player 	- numer gracza, na rzecz którego wykonywane jest sprawdzenie,
 * @param[in] x 		- współrzędna pozioma sprawdzanego pola,
 * @param[in] y 		- współrzędna pionowa sprawdzanego pola.
 * @return Wartość @p true, jeśli pole o zadanych współrzędnych sąsiaduje z
 * jakimkolwiek polem należącym do podanego gracza lub @p false, jeśli wśród
 * sąsiadów nie ma pól należących do gracza.
 */
bool player_fields_around(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
	if((x != 0 && g->board[x - 1][y] == player)//lewy sąsiad
	   || (x != UINT32_MAX && x + 1 < g->width && g->board[x + 1][y] == player)//prawy sąsiad
	   || (y != 0 && g->board[x][y - 1] == player)//dolny sąsiad
	   || (y != UINT32_MAX && y + 1 < g->height && g->board[x][y + 1] == player)) {//górny sąsiad
		return true;
	}
	else {
		return false;
	}
}

/** @brief Liczy jakie pola może jeszcze zająć gracz, ktory osiągnał limit obszarów.
 * Funkcja przchodzi całą planszę i liczy pola, które sąsiadują z jakimkolwiek
 * polem należącym do danego gracza.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player 	- numer gracza, dla którego chcemy policzyć pola.
 * @return Liczba pól możliwych do zajęcia przez gracza, który osiągnął limit
 * obszarów, liczba nieujemna.
 */
uint64_t count_free_fields(gamma_t *g, uint32_t player) {
	uint64_t counter = 0;
	for(uint32_t x = 0; x < g->width; x++) {
		for(uint32_t y = 0; y < g->height; y++) {
			if(player_fields_around(g, player, x, y)
			   && g->board[x][y] == 0) {
				counter++;
			}
		}
	}
	return counter;
}


/** @brief Dodaje pole do kolejki.
 * Funkcja dodaje do kolejki sąsiada podanego pola. O tym, którego z sąsiadów,
 * decydują wartości parametrów @p coordinate i @p factor.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in,out] visited - plansza używana do oznaczania pól, które były wcześniej
 * 						dodane do kolejki,
 * @param[in,out] queue - wskaźnik na kolejkę,
 * @param[in] field 	- wskaźnik na strukurę pola, którego sąsiad ma
 * 						znaleźć się w kolejce,
 * @param[in] player 	- numer gracza, którego obszar jest przedmiotem przeszukiwania
 * 						metodą BFS,
 * @param[out] coordinate - wskaźnik na współrzędną(x lub y), będącą zmienną
 * 						struktury pola, która ma zostać zmodyfikowana,
 * @param[in] factor 	- czynnik o jaki należy zmodyfikować podaną współrzędną.
 */
void add_to_queue(gamma_t *g, bool **visited, queue_t *queue, field_t *field,
                  uint32_t player, uint32_t *coordinate, int factor) {
	(*coordinate) += factor;
	if(visited[field->x][field->y] == false
	   && g->board[field->x][field->y] == player)  {
		enqueue(queue, *field);
		visited[field->x][field->y] = true;
	}
	(*coordinate) -= factor;
}

/** @brief Dodaje do kolejki wszystkie sąsiednie pola.
 * Korzysta z funkcji pomocniczej @ref add_to_queue, dodającej pojedyncze
 * pola-sąsiadów do kolejki.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in,out] visited - plansza używana do oznaczania pól, które były wcześniej
 * 						dodane[in] do kolejki,
 * @param[in,out] queue - wskaźnik na kolejkę,
 * @param[in] field 	- pole, którego sąsiedzi mają być dodani do kolejki,
 * @param[in] player 	- numer gracza, którego obszar jest przedmiotem
 * 						przeszukiwania metodą BFS.
 */
void add_to_queue_fields_around(gamma_t *g, bool **visited, queue_t *queue,
                                field_t field, uint32_t player) {
	
	if((field.x) != 0) {
		add_to_queue(g, visited, queue, &field, player, &(field.x), -1);
	}
	
	if(field.x != UINT32_MAX && field.x + 1 < g->width) {//prawy sąsiad
		add_to_queue(g, visited, queue, &field, player, &(field.x), 1);
	}
	
	if((field.y) != 0) {//dolny sąsiad
		add_to_queue(g, visited, queue, &field, player, &(field.y), -1);
	}
	
	if(field.y != UINT32_MAX && field.y + 1 < g->height) {//górny sąsiad
		add_to_queue(g, visited, queue, &field, player, &(field.y), 1);
	}
}

/** @brief Wykonuje przeszukiwanie metodą BFS.
 * Funkcja najpierw dodaje do kolejki pierwsze pole, a potem pętli wszystkie
 * kolejne za pomocą funkcji @ref add_to_queue_fields_around. Pętla konczy się
 * w momencie opróżnienia kolejki.
 * Celem funkcji jest oznaczenie pól jednego obszaru na planszy @p visited.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player 	- numer gracza, którego obszar jest przedmiotem
 * 						przeszukiwania metodą BFS,
 * @param[in,out] visited - plansza używana do oznaczania pól, które były
 * 						wcześniej dodane do kolejki,
 * @param[in] field 	- pole, od którego po zostać zaczęte przeszukiwanie BFS.
 */
void make_BFS(gamma_t *g, uint32_t player, bool **visited, field_t field) {
	queue_t queue = init_queue();
	enqueue(&queue, field);
	visited[field.x][field.y] = true;
	do {
		if(!empty_queue(queue)) {
			field = dequeue(&queue);
		}
		add_to_queue_fields_around(g, visited, &queue, field, player);
	} while(!empty_queue(queue));
	
	clear_queue(&queue);
}

/** @brief Sprawdza czy pole należy do nowego obszaru.
 * Funkcja wywołuje BFS dla sąsiada podanego pola. O tym, dla którego z sąsiadów,
 * decydują wartości parametrów @p coordinate i @p factor.
 * BFS jest wywołany jeśli zadany sąsiad jest częścią nowego,
 * dotąd nieoznaczonego obszaru. Jeśli pole sąsiada zostało oznaczone na planszy
 * @p visited to znaczy, że należy innego, już policzonego obszaru.
 * Wtedy BFS dla takiego pola jest pomijany.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in,out] visited - plansza używana do oznaczania pól, które były wcześniej
 * 						dodane do kolejki,
 * @param[in] field 	- wskaźnik na strukurę pola, którego sąsiad ma
 * 						znaleźć się w kolejce,
 * @param[in] player 	- numer gracza, którego obszary chcemy sprawdzić,
 * @param[in] coordinate - wskaźnik na współrzędną(x lub y), będącą zmienną
 * 						struktury pola, która ma zostać zmodyfikowana,
 * @param[in] factor 	- czynnik o jaki należy zmodyfikować podaną współrzędną.
 * @return Wartość 1, jeśli pole należy do gracza i nie zostało oznaczone
 * wcześniej, czyli należy do nowego obszaru lub 0, jeśli nie spelnia tych
 * warunków.
 */
bool is_another_area(gamma_t *g, bool **visited, field_t *field,
                     uint32_t player, uint32_t *coordinate, int factor) {
	bool another_area = false;
	(*coordinate) += factor;
	if(visited[field->x][field->y] == false
	   && g->board[field->x][field->y] == player)  {
		make_BFS(g, player, visited, *field);
		another_area = true;
	}
	(*coordinate) -= factor;
	return another_area;
}

/** @brief Liczy wszystkie gracza obszary znajdujące się wokół podanego pola.
 * W ten sposób sprawdzamy ile obszarów gracza zostanie połączonych w momencie
 * wykonania ruchu na pole o danych współrzędnych @p x i @p y. Dla każdego z
 * sąsiednich pól wywoływane jest osobne sprawdzenie czy jest częscią kolejnego,
 * nieoznaczonego obszaru. Jeśli pole zostało oznaczone na planszy @p visited
 * to znaczy, że należy do tego samego obszaru co któreś z innych sąsiednich pól.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player 	- numer gracza, którego obszary chcemy sprawdzić,
 * @param[in] x 		- współrzędna pionowa pola na planszy.
 * @param[in] y 		- współrzędna pionowa pola na planszy.
 * @return Wartość od 0 do 4 (przy czym wartość 0 przy normalnym użyciu nigdy nie
 * zostanie uzyskana ze względu na istnienie funkcji @ref player_fields_around).
 * Wartość oznacza liczbę obszarów gracza, z którymi sąsiaduje pole.
 */
int number_of_connected_areas(gamma_t *g, uint32_t player, uint32_t x,
                              uint32_t y) {
	bool **visited = (bool **)init_board(sizeof(uint8_t), g->width,
	                                             g->height);
	int counter = 0;
	field_t field;
	field.x = x;
	field.y = y;
	
	if(field.x != 0) {//obszar lewego sąsiada
		counter += is_another_area(g, visited, &field, player, &(field.x), -1);
	}
	
	if(field.x != UINT32_MAX && field.x + 1 < g->width) {//obszar prawego sąsiada
		counter += is_another_area(g, visited, &field, player, &(field.x), 1);
	}
	
	if(field.y != 0) {//obszar dolnego sąsiada
		counter += is_another_area(g, visited, &field, player, &(field.y), -1);
	}
	
	if(field.y != UINT32_MAX && field.y + 1 < g->height) {//obszar górnego sąsiada
		counter += is_another_area(g, visited, &field, player, &(field.y), 1);
	}
	
	delete_board((void **)visited, g->width);
	return counter;
}

/** @brief Wywołuje funkcję liczącą obszary właściciela na planszy.
 * W ten sposób sprawdzamy ile obszarów właściciela zostanie rozdzielonych przy
 * złotym ruchu innego gracza na dane pole. Liczymy to tak samo jak liczbę
 * obszarów połączonych przez normalny ruch, przy czym przed wywołaniem funkcji
 * @ref number_of_connected_areas chwilowo ustawiamy dane pole na puste.
 * @param[in] g 	- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] owner - obecny właściciel pola,
 * @param[in] x 	- współrzędna pozioma pola na planszy,
 * @param[in] y 	- współrzędna pionowa pola na planszy.
 * @return Wartość od 0 do 4, w  zależności ile obszarów właściciel zostanie
 * rozdzielonych.
 */
int number_of_split_areas(gamma_t *g, uint32_t owner, uint32_t x, uint32_t y) {
	g->board[x][y] = 0;
	int number = number_of_connected_areas(g, owner, x, y);
	g->board[x][y] = owner;
	return number;
}


/** @brief Tworzy napis odpowiadający zawartości planszy.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry.
 * @return Wskaźnik na zaalokowany napis lub NULL, jeśli zabrakło pamięci.
 */
char* standard_board(gamma_t *g) {
	uint32_t width = g->width;
	uint32_t height = g->height;
	int k = 0;
	size_t memory_amount = ((width + 1) * height) + 1;
	char *gamma_board = calloc(memory_amount, sizeof(char));
	if(!gamma_board) {
		errno = 0;
		return NULL;
	}
	
	for(int i = (int)height - 1; i >= 0; i--) {
		for(uint32_t j = 0; j < width; j++) {
			if(g->board[j][i] != 0) {
				gamma_board[k] = g->board[j][i] + '0';
			}
			else {
				gamma_board[k] = '.';
			}
			k++;
		}
		gamma_board[k] = '\n';
		k++;
	}
	gamma_board[k] = '\0';
	
	return gamma_board;
}

/** @brief Tworzy napis opisujący stan planszy, wersja dla liczby graczy >9.
 * W przypadku, gdy liczba graczy ma więcej niż 1 cyfrę, kolumny
 * planszy oddzielamy spacją.
 * @param[in] g 				- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] max_num_of_digits - maksymalna ilość cyfr w numerze gracza.
 * @return Wskaźnik na zaalokowany napis lub NULL, jeśli zabrakło pamięci.
 */
char* board_for_multiple_digits_players(gamma_t *g, size_t max_num_of_digits) {
	uint32_t width = g->width;
	uint32_t height = g->height;
	int k = 0;
	size_t memory_amount = (((max_num_of_digits + 1) * width + 1) * height) + 1;
	char *gamma_board = calloc(memory_amount, sizeof(char));
	if(!gamma_board) {
		errno = 0;
		return NULL;
	}
	
	for(int i = (int)height - 1; i >= 0; i--) {
		for(uint32_t j = 0; j < width; j++) {
			if(g->board[j][i] != 0) {
				char num_str[UINT_LIMIT];
				size_t num_of_digits = number_of_digits(g->board[j][i]);
				sprintf(num_str, "%u", g->board[j][i]);
				strcat(gamma_board, num_str);
				k += num_of_digits;
				for(uint32_t l = 0; l <= max_num_of_digits - num_of_digits; l++) {
					gamma_board[k] = ' ';
					k++;
				}
			}
			else {
				gamma_board[k] = '.';
				k++;
				for(uint32_t l = 0; l < max_num_of_digits; l++) {
					gamma_board[k] = ' ';
					k++;
				}
			}
		}
		gamma_board[k] = '\n';
		k++;
	}
	gamma_board[k] = '\0';
	
	return gamma_board;
}


uint32_t gamma_players(gamma_t *g) {
	return g->players;
}

uint32_t gamma_width(gamma_t *g) {
	return g->width;
}

uint32_t gamma_height(gamma_t *g) {
	return g->height;
}

gamma_t* gamma_new(uint32_t width, uint32_t height,
					uint32_t players, uint32_t areas) {
	if(width <= 0 || height <= 0 || players <= 0 || areas <= 0
	|| (uint64_t)width * (uint64_t)height > BOARD_SIZE_HARD_LIMIT) {
		return NULL;
	}

	gamma_t *g = malloc(sizeof(gamma_t));
	if(!g) {
		errno = 0;
		return NULL;
	}

	g->board = (uint32_t **)init_board(sizeof(uint32_t), width, height);
	g->busy_fields = calloc((size_t)players + 1, sizeof(uint64_t));
	g->free_fields = calloc((size_t)players + 1,  sizeof(uint64_t));
	g->areas = calloc((size_t)players + 1, sizeof(uint32_t));
	g->golden_move_available = calloc((size_t)players + 1, sizeof(bool));
	
	g->all_free_fields = (uint64_t)width * (uint64_t)height;
	g->width = width;
	g->height = height;
	g->players = players;
	g->max_areas = areas;
	
	if(!(g->board) || !(g->busy_fields) || !(g->free_fields)
		|| !(g->areas) || !(g->golden_move_available)) {
		errno = 0;
		gamma_delete(g);
		return NULL;
	}
	init_boolean_array(g->golden_move_available, players + 1);
	
	return g;
}

void gamma_delete(gamma_t *g) {
	if(g != NULL) {
		delete_board((void **)g->board, g->width);
		free(g->busy_fields);
		free(g->free_fields);
		free(g->areas);
		free(g->golden_move_available);
		free(g);
	}
}

bool gamma_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
	if(g == NULL || player > (g->players) || player <= 0 || x >= (g->width)
		|| y >= (g->height)) {
		return false;
	}

	if ((g->board[x][y] != 0)
		|| (!player_fields_around(g, player, x, y)
			&& g->areas[player] == g->max_areas)) {
		return false;
	}
	else {
		if(!player_fields_around(g, player, x, y)) {
			(g->areas[player])++;
		}
		else {
			g->areas[player] -= number_of_connected_areas(g, player, x, y) - 1;
		}
		(g->busy_fields[player])++;
		(g->all_free_fields)--;
		g->board[x][y] = player;
		return true;
	}
}

bool gamma_golden_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
	if(g == NULL || player > (g->players) || player <= 0 || x >= (g->width)
		|| y >= (g->height)) {
		return false;
	}

	uint32_t current_owner = g->board[x][y];
	int var_number_of_split_areas = number_of_split_areas(g, current_owner, x, y);
	
	if(!(g->golden_move_available[player])
		|| g->board[x][y] == 0
		|| g->board[x][y] == player
		|| (!player_fields_around(g, player, x, y) && g->areas[player] == g->max_areas)
		|| (g->areas[current_owner] + var_number_of_split_areas - 1 > g->max_areas)) {
		return false;
	}
	else {
		if(!player_fields_around(g, player, x, y)) {
			(g->areas[player])++;
		}
		else {
			g->areas[player] -= number_of_connected_areas(g, player, x, y) - 1;
		}
		g->areas[current_owner] += var_number_of_split_areas - 1;
		(g->busy_fields[current_owner])--;
		(g->busy_fields[player])++;
		g->golden_move_available[player] = false;
		g->board[x][y] = player;
		return true;
	}
}

uint64_t gamma_busy_fields(gamma_t *g, uint32_t player) {
	if(g == NULL || player > g->players || player <= 0) {
		return 0;
	}
	else {
		return g->busy_fields[player];
	}
}

uint64_t gamma_free_fields(gamma_t *g, uint32_t player) {
	if(g == NULL || player > g->players || player <= 0) {
		return 0;
	}
	else if(g->areas[player] < g->max_areas) {
		return g->all_free_fields;
	}
	else {
		return count_free_fields(g, player);
	}
}

bool gamma_golden_possible(gamma_t *g, uint32_t player) {
	if(g == NULL || player > g->players || player <= 0) {
		return false;
	}

	if(g->golden_move_available[player]
		&& busy_fields_available(g->busy_fields, g->players, player)) {
		return true;
	}
	else {
		return false;
	}
}

char* gamma_board(gamma_t *g) {
	if(g == NULL) {
		return NULL;
	}
	
	char *gamma_board = NULL;
	int max_num_of_digits = number_of_digits(g->players);
	
	if(max_num_of_digits == 1) {
		gamma_board = standard_board(g);
	}
	else {
		gamma_board = board_for_multiple_digits_players(g, max_num_of_digits);
	}

	return gamma_board;
}
