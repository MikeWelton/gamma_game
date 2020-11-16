/** @file
 * Implementacja klasy obsługującej tryb interaktywny (interactive mode).
 *
 * @author Michał Orzyłowski <mo418334@students.mimuw.edu.pl>
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "interactive_mode.h"
#include "auxiliary_functions.h"


#define EOT 4 	/**< End of Transmission, znak zwracany przez @ref getch
 * 				po wciśnięciu Ctrl + D */

/** @name Makra zastępujące kody ANSI
 */
///@{
#define MOVE_CURSOR_UP "\x1b[A"			///< Przesunięcie w górę kursora w terminalu
#define MOVE_CURSOR_DOWN "\x1b[B"		///< Przesunięcie w dół kursora w terminalu
#define MOVE_CURSOR_FORWARD "\x1b[C"	/**< Przesunięcie do przodu (w prawo) kursora
 *											 w terminalu */
#define MOVE_CURSOR_BACK "\x1b[D"		/**< Przesunięcie do tyłu (w lewo) kursora
 * 											w terminalu */

#define MOVE_CURSOR_TO_TOP_LEFT "\x1b[H"/**< Przesunięcie kursora w lewy, górny róg
 * 											 terminala */

#define SAVE_CURSOR_POSITION "\x1b[s"	///< Zapisanie pozycji kursora w terminalu
#define RESTORE_CURSOR_POSITION "\x1b[u"///< Przywrócenie pozycji kursora w terminalu

#define CLEAR_DISPLAY "\x1b[0J"			/**< Usunięcie całej zawartości wyświetlanego
 * 										 	 okna terminala */
#define CLEAR_LINE "\x1b[2K"			///< Usunięcie całej linijki w terminalu


#define MOVE_CURSOR_MANY_CELLS_FORWARD(num_of_cells) "\x1b[%uC", num_of_cells /**<
 * Przesunięcie kursora do przodu (w prawo) o zadaną liczbę komórek.
 * Uwaga: Makro nie może być używane z innymi makrami w jednym wywołaniu funkcji
 * printf. Żeby działało poprawnie należy je umieścić w osobnym wywołaniu funkcji
 * printf.
 */
 
#define MOVE_CURSOR_MANY_CELLS_BACK(num_of_cells) "\x1b[%uD", num_of_cells /**<
 * Przesunięcie kursora do tyłu (w lewo) o zadaną liczbę komórek.
 * Uwaga: Makro nie może być używane z innymi makrami w jednym wywołaniu funkcji
 * printf. Żeby działało poprawnie należy je umieścić w osobnym wywołaniu funkcji
 * printf.
 */
 
#define MOVE_CURSOR_MANY_LINES_DOWN(num_of_lines) "\x1b[%uE", num_of_lines /**<
 * Przesunięcie kursora w dół o zadaną liczbę linii, na początek linijki.
 * Uwaga: Makro nie może być używane z innymi makrami w jednym wywołaniu funkcji
 * printf. Żeby działało poprawnie należy je umieścić w osobnym wywołaniu funkcji
 * printf.
 */

#define ACTIVATE_REVERSE_FG_BG "\x1b[7m" /**< Kod ANSI aktywujący zamianę koloru
 * tła z kolorem tekstu.
 */
 
#define RESET "\x1b[0m" /**< Resetuje poprzednio aktywowane kody ANSI modyfikujące
 * tekst.
 */
///@}


static char getch();

static void show_board(gamma_t *g);

static void show_player_info(gamma_t *g, uint32_t player,
											uint64_t free_fields, uint32_t y);

static void print_summary(gamma_t *g, uint32_t y);

static void manage_highlight(gamma_t *g, uint32_t x, uint32_t y, bool add);

static void add_highlight(gamma_t *g, uint32_t x, uint32_t y);

static void delete_highlight(gamma_t *g, uint32_t x, uint32_t y);

static void move_cursor_up(gamma_t *g, const uint32_t *x, uint32_t *y);

static void move_cursor_down(gamma_t *g, const uint32_t *x, uint32_t *y);

static void move_cursor_forward(gamma_t *g, uint32_t *x, const uint32_t *y);

static void move_cursor_back(gamma_t *g, uint32_t *x, const uint32_t *y);

static uint32_t count_real_x(gamma_t *g, uint32_t x);

static void read_player_input(gamma_t *g, uint32_t player, uint32_t *x,
														uint32_t *y, bool *end);

static void simulate_turn(gamma_t *g, bool *end, uint32_t *x, uint32_t *y);

static void get_window_size(uint32_t *terminal_width, uint32_t *terminal_height);

static bool board_to_large(gamma_t *g);


/** @brief Czyta znaki wprowadzane w konsoli.
 * Funkcja czyta w sposób ciągły kolejne znaki wprowadzane w konsoli.
 * @return Pobrany znak.
 */
char getch() {
	char c;
	struct termios new, old;
	
	tcgetattr(STDIN_FILENO, &old);
	new = old;
	new.c_lflag &= ~ICANON;
	new.c_lflag &= ~ECHO;
	
	tcsetattr(STDIN_FILENO, TCSANOW, &new);
	c = (char)getchar();
	
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return c;
}

/** @brief Wypisuje planszę z obecnym stanem rozgrywki.
 * Zachowuje położenie kursora tzn. po zakończeniu działania funkcji
 * jego położenie jest takie same jak przed wypisaniem.
 * @param[in] g 			- wskaźnik na strukturę przechowującą stan gry.
 */
void show_board(gamma_t *g) {
	char *board = gamma_board(g);
	if(board == NULL) {
		return;
	}
	printf(SAVE_CURSOR_POSITION MOVE_CURSOR_TO_TOP_LEFT CLEAR_DISPLAY);
	printf("%s", board);
	printf(RESTORE_CURSOR_POSITION);
	free(board);
}

/** @brief Wypisuje informacje dotyczące danego gracza.
 * Przesuwa kursor do pierwszej lini nie zajętej przez planszę i wypisuje
 * informacje dotyczące danego gracza. Zachowuje położenie kursora tzn.
 * po zakończeniu działania funkcji jego położenie jest takie same jak przed
 * wypisaniem.
 * @param[in] g 			- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player		- numer gracza, którego informacje wypisujemy,
 * @param[in] free_fields	- liczba wolnych pól, które może zająć dany gracza,
 * @param[in] y 			- wartość współrzędnej pionowej w terminalu.
 */
void show_player_info(gamma_t *g, uint32_t player, uint64_t free_fields, uint32_t y) {
	uint64_t busy_fields = gamma_busy_fields(g, player);
	uint32_t num_of_lines = gamma_height(g) - y + 1;
	
	printf(SAVE_CURSOR_POSITION MOVE_CURSOR_MANY_LINES_DOWN(num_of_lines));
	printf(CLEAR_LINE);
	printf("PLAYER %d BUSY_FIELDS %lu FREE_FIELDS %lu", player,
			busy_fields, free_fields);
	if(gamma_golden_possible(g, player)) {
		printf(" GOLDEN_MOVE_AVAILABLE");
	}
	printf("\n");
	printf(RESTORE_CURSOR_POSITION);
}

/** @brief Wypisuje podsumowanie gry.
 * Przesuwa kursor do pierwszej lini nie zajętej przez planszę i wypisuje
 * podsumowanie gry dla wszystkich graczy.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] y 		- wartość współrzędnej pionowej w terminalu.
 */
void print_summary(gamma_t *g, uint32_t y) {
	uint32_t num_of_lines = gamma_height(g) - y + 1;
	
	printf(MOVE_CURSOR_MANY_LINES_DOWN(num_of_lines));
	printf(CLEAR_LINE);
	uint32_t players = gamma_players(g);
	for(uint32_t i = 1; i <= players; i++) {
		uint64_t owned_fields = gamma_busy_fields(g, i);
		printf("PLAYER %d OWNED_FIELDS %lu\n", i, owned_fields);
	}
}

/** @brief Dodaje lub usuwa podświetlenie.
 * Funkcja, w zależności od wartości zmiennej add, dodaje lub usuwa
 * podświetlenie. Zmienna lokalna position to numer znaku w napisie z
 * planszą, na którym obecnie znajduje się kursor w terminalu. Wartość ta wynika z
 * przyjętego, w @ref gamma_board, sposobu wypisywania graczy o wielu cyfrach.
 * Dodając lub usuwając podświetlenie wypisujemy (num_of_digits - 1) znaków z
 * napisu, zaczynając od znaku pod numerem (position + 1), ponieważ pierwszy znak
 * numeru gracza będzie zawsze podświetlony przez kursor. W ten sposób nie
 * wypisujemy po raz kolejny całej planszy, a jednynie kawałek wymagający
 * modyfikacji.
 * @param[in] g 				- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] x 				- aktualna wartość współrzędnej poziomej położenia
 * 								kursora w terminalu,
 * @param[in] y 				- aktualna wartość współrzędnej pionowej położenia
 * 								kursora w terminalu,
 * @param[in] add 				- zmienna, której wartość określa sposób
 * działania funkcji. Jeśli ma wartość true to funkcja dodaje podświetlenie,
 * jeśli false usuwa.
 */
void manage_highlight(gamma_t *g, uint32_t x, uint32_t y, bool add) {
	char *board = gamma_board(g);
	if(board == NULL) {
		return;
	}
	uint32_t players = gamma_players(g),
			width = gamma_width(g);
	int num_of_digits = number_of_digits(players);
	uint64_t position = (y - 1) * (width * (num_of_digits + 1) + 1) + x;
	
	printf(SAVE_CURSOR_POSITION);
	
	printf(MOVE_CURSOR_FORWARD);
	if(add) {
		printf(ACTIVATE_REVERSE_FG_BG "%.*s" RESET,
			   num_of_digits - 1, board + position);
	}
	else { //delete_highlight
		printf("%.*s",
			   num_of_digits - 1, board + position);
	}
	
	printf(RESTORE_CURSOR_POSITION);
	free(board);
}

/** @brief Dodaje podświetlenie.
 * Funkcja dodaje podświetlenie wołając funkcję @ref manage_highlight z odpowiednią
 * wartością ostatniego argumentu.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[out] x		- aktualna wartość współrzędnej poziomej położenia
 * 						kursora w terminalu,
 * @param[out] y 		- aktualna wartość współrzędnej pionowej położenia
 * 						kursora w terminalu.
 */
void add_highlight(gamma_t *g, uint32_t x, uint32_t y) {
	manage_highlight(g, x, y, true);
}

/** @brief Usuwa podświetlenie.
 * Funkcja usuwa podświetlenie wołając funkcję @ref manage_highlight z odpowiednią
 * wartością ostatniego argumentu.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[out] x 		- aktualna wartość współrzędnej poziomej położenia
 * 						kursora w terminalu,
 * @param[out] y 		- aktualna wartość współrzędnej pionowej położenia
 * 						kursora w terminalu.
 */
void delete_highlight(gamma_t *g, uint32_t x, uint32_t y) {
	manage_highlight(g, x, y, false);
}

/** @brief Przesuwa kursor w górę.
 * Przesuwa kursor w górę, o ile nie jest już na krawędzi planszy.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[out] x		- wskaźnik na aktualną wartość współrzędnej poziomej
 * 						położenia kursora w terminalu,
 * @param[in,out] y 	- wskaźnik na aktualną wartość współrzędnej pionowej
 * 						położenia kursora w terminalu.
 */
void move_cursor_up(gamma_t *g, const uint32_t *x, uint32_t *y) {
	if((*y) > 1) {
		if(gamma_players(g) > 9) {
			delete_highlight(g, *x, *y);
		}
		printf(MOVE_CURSOR_UP);
		--(*y);
	}
}

/** @brief Przesuwa kursor w dół.
 * Przesuwa kursor w dół, o ile nie jest już na krawędzi planszy.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[out] x		- wskaźnik na aktualną wartość współrzędnej poziomej
 * 						położenia kursora w terminalu,
 * @param[in,out] y 	- wskaźnik na aktualną wartość współrzędnej pionowej
 * 						położenia kursora w terminalu.
 */
void move_cursor_down(gamma_t *g, const uint32_t *x, uint32_t *y) {
	if((*y) < gamma_height(g)) {
		if(gamma_players(g) > 9) {
			delete_highlight(g, *x, *y);
		}
		printf(MOVE_CURSOR_DOWN);
		(*y)++;
	}
}

/** @brief Przesuwa kursor do przodu (w prawo).
 * Przesuwa kursor do przodu (w prawo), o ile nie jest już na krawędzi planszy.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in,out] x 	- wskaźnik na aktualną wartość współrzędnej poziomej
 * 						położenia kursora w terminalu,
 * @param[out] y 		- wskaźnik na aktualną wartość współrzędnej pionowej
 * 						położenia kursora w terminalu.
 */
void move_cursor_forward(gamma_t *g, uint32_t *x, const uint32_t *y) {
	uint32_t players = gamma_players(g);
	uint32_t num_of_digits = number_of_digits(players);
	if(players > 9 && (*x)/(num_of_digits + 1) + 1 < gamma_width(g)) {
		delete_highlight(g, *x, *y);
		
		printf(MOVE_CURSOR_MANY_CELLS_FORWARD(num_of_digits + 1));
		(*x) += num_of_digits + 1;
	}
	else if((*x) < gamma_width(g)) {
		printf(MOVE_CURSOR_FORWARD);
		(*x)++;
	}
}

/** @brief Przesuwa kursor do tyłu (w lewo).
 * Przesuwa kursor do tyłu (w lewo), o ile nie jest już na krawędzi planszy.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in,out] x 	- wskaźnik na wartość współrzędnej poziomej w terminalu,
 * @param[out] y		- wskaźnik na aktualną wartość współrzędnej pionowej
 * 						położenia kursora w terminalu.
 */
void move_cursor_back(gamma_t *g, uint32_t *x, const uint32_t *y) {
	uint32_t players = gamma_players(g);
	uint32_t num_of_digits = number_of_digits(players);
	if(players > 9 && (*x)/(num_of_digits + 1) + 1  > 1) {
		delete_highlight(g, *x, *y);
		
		printf(MOVE_CURSOR_MANY_CELLS_BACK(num_of_digits + 1));
		(*x) -= num_of_digits + 1;
	}
	else if((*x) > 1) {
		printf(MOVE_CURSOR_BACK);
		--(*x);
	}
}

/** @brief Liczy wartość współrzędnej x na planszy.
 * Funkcja przekształca wartość współrzędnej x z układu terminala na wartość
 * tej współrzędnej w układzie odniesienia planszy. Wzór na x bierze się stąd,
 * że dla >9 graczy na planszy, początek n-tego numeru gracza w linijce
 * znajduje się na pozycji (n - 1) * (num_of_digits + 1) + 1.
 * @param g 				- wskaźnik na strukturę przechowującą stan gry,
 * @param x 				- współrzędna pozioma położenia kursora w terminalu,
 * @return Wartość współrzędnej x w układzie odniesienia planszy.
 */
uint32_t count_real_x(gamma_t *g, uint32_t x) {
	uint32_t players = gamma_players(g);
	uint32_t num_of_digits = number_of_digits(players);
	if(players > 9) {
		x = x/(num_of_digits + 1) + 1;
	}
	return x - 1;
}

/** @brief Czyta dane wejściowe z terminala i umożliwia wykonanie ruchu.
 * Pozwala na wykonanie jednego ruchu (@ref gamma_move lub @ref gamma_golden_move),
 * przez jednego gracza.
 * Czyta w sposób ciągły, za pomocą funkcji @ref getch, wartości klawiszy
 * wciskanych przez użytkownika w czasie gry. Wywołuje odpowiednie funkcje,
 * przesuwające kursor lub wykonujące ruchu na planszy.
 * Zmienne real_x i real_y, deklarowane w funkcji, zapisują położenie kursora
 * na planszy (według układu odniesienia planszy, a nie terminala) i to one są
 * przekazywane do funkcji silnika, w celu wykonia ruchu na dane pole.
 * @param[in,out] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in] player 		- numer gracza, który ma wykonać ruch,
 * @param[in,out] x 		- współrzędna pozioma położenia kursora w terminalu,
 * @param[in,out] y 		- współrzędna pionowa położenia kursora w terminalu,
 * @param[in,out] end 		- zmienna zapisująca informację o konieczności
 * 							zakończenia gry, w tej funkcji może przyjąć wartość
 * 							@p true, gdy gracz wciśnie Ctrl + D.
 */
void read_player_input(gamma_t *g, uint32_t player, uint32_t *x, uint32_t *y,
																	bool *end) {
	uint32_t real_x, real_y;
	bool move_made = false;
	char first, second, third;
	
	first = second = '\0';
	while(!move_made) {
		if(gamma_players(g) > 9) {
			add_highlight(g, *x, *y);
		}
		real_x = count_real_x(g, *x);
		real_y = gamma_height(g) - (*y);
		third = getch();
		if(third == EOT) {
			(*end) = true;
			move_made = true;
		}
		else if(third == ' ') {
			move_made = gamma_move(g, player, real_x, real_y);
		}
		else if(third == 'g' || third == 'G') {
			move_made = gamma_golden_move(g, player, real_x, real_y);
		}
		else if(first == '\x1b' && second == '[' && third == 'A') {
			move_cursor_up(g, x, y);
		}
		else if(first == '\x1b' && second == '[' && third == 'B') {
			move_cursor_down(g, x, y);
		}
		else if(first == '\x1b' && second == '[' && third == 'C') {
			move_cursor_forward(g, x, y);
		}
		else if(first == '\x1b' && second == '[' && third == 'D') {
			move_cursor_back(g, x, y);
		}
		else if(third == 'c' || third == 'C') {
			move_made = true;
		}
		first = second;
		second = third;
	}
}

/** @brief Symuluje jedną turę rozgrywki.
 * Funkcja przeprowadza jedną turę rozgrywki tzn. zarządza wykonaniem ruchu przez
 * wszyskich kolejnych graczy. Każdorazowo sprawdza czy dany gracz może wykonać
 * ruch. Jeśli tak, funkcja rozpoczyna czytanie wciskanych klawiszy,
 * w przeciwnym wypadku sprawdzany jest następny gracz. Funkcja kończy działanie,
 * gdy został wciśnięty klawisz kończący rozgrywkę lub gdy żaden z graczy nie może
 * już wykonać ruchu.
 * @param[in] g 		- wskaźnik na strukturę przechowującą stan gry,
 * @param[in,out] end 	- zmienna zapisująca informację czy należy zakończyć grę,
 * @param[in,out] x 	- wskaźnik na wartość współrzędnej poziomej w terminalu.
 * @param[in,out] y 	- wskaźnik na wartość współrzędnej pionowej w terminalu.
 */
void simulate_turn(gamma_t *g, bool *end, uint32_t *x, uint32_t *y) {
	uint32_t player = 1,
	num_of_players = gamma_players(g),
	players_without_move = 0;
	uint64_t free_fields;
	bool golden_possible;
	while(player <= num_of_players && !(*end)) {
		free_fields = golden_possible = 0;
		while(free_fields == 0 && !golden_possible && player <= num_of_players) {
			free_fields = gamma_free_fields(g, player);
			golden_possible = gamma_golden_possible(g, player);
			if(free_fields == 0 && !golden_possible) {
				players_without_move++;
				player++;
			}
		}
		if(player <= num_of_players) {
			show_board(g);
			show_player_info(g, player, free_fields, *y);
			read_player_input(g, player, x, y, end);
			player++;
		}
		else if(players_without_move == num_of_players) {
			(*end) = true;
		}
	}
}

/** @brief Sprawdza wielkość terminala i zapisuje wymiary na zmienne.
 * @param[out] terminal_width 	- zmienna, na którą zapisujemy szerokość terminala,
 * @param[out] terminal_height 	- zmienna, na którą zapisujemy wysokość terminala.
 */
void get_window_size(uint32_t *terminal_width, uint32_t *terminal_height) {
	struct winsize winsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
	
	*terminal_width = winsize.ws_col;
	*terminal_height = winsize.ws_row;
}

/** @brief Sprawdza czy plansza nie jest zbyt duża.
 * Sprawdza czy plansza nie przekracza wymiarów terminala pomniejszonych o
 * zapasowe 5 komórek.
 * @param[in] g 	- wskaźnik na strukturę przechowującą stan gry,
 * @return Wartość @p true, jeśli wymiary planszy przekraczają rozmiar okna
 * terminala pomniejszony o 5 komórek zapasu. Wartość @p false, w przeciwnym
 * wypadku, gdy żądane wymiary planszy mieszczą się w limicie.
 */
bool board_to_large(gamma_t *g) {
	const int margin = 5; //zostawiamy zawsze 5 pól zapasu na brzegach terminalu
	uint64_t board_width;
	uint32_t board_height = gamma_height(g),
	players = gamma_players(g),
	num_of_digits = number_of_digits(players),
	terminal_width, terminal_height;
	get_window_size(&terminal_width, &terminal_height);
	if(num_of_digits > 1) {
		board_width = (uint64_t)gamma_width(g) * ((uint64_t)num_of_digits + 1);
	}
	else {
		board_width = (uint64_t)gamma_width(g);
	}
	if((uint64_t)board_width + margin > terminal_width
	|| (uint64_t)board_height + margin > terminal_height) {
		printf(MOVE_CURSOR_TO_TOP_LEFT CLEAR_DISPLAY);
		printf("Cannot create the game. Chosen board is too large for "
		 "terminal window.\n");
		printf("Please, choose smaller one, change font size or/and resize"
		 " terminal window.\n" "Ending program.\n");
		return true;
	}
	return false;
}

int interactive_mode(gamma_t *g) {
	if(board_to_large(g)) {
		return 1;
	}
	
	bool end = false;
	printf(MOVE_CURSOR_TO_TOP_LEFT);
	uint32_t x = 1, y = 1; //współrzędne kursora w terminalu, nie na planszy!!!
	while(!end) {
		simulate_turn(g, &end, &x, &y);
	}
	show_board(g);
	print_summary(g, y);
	return 0;
}
