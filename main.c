#include <stdlib.h>
#include <curses.h>
#include <locale.h>

WINDOW *master;
WINDOW *gamewin;
WINDOW *playerfield; 
WINDOW *enemyfield;
WINDOW *playerfleet;
WINDOW *enemyfleet;

void init_curses(){
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	master = newwin(0,0,0,0);
	refresh();
}

void print_title_screen(){
	
	int title_len = 48;
	char title[3][128] = {"▒█▀▀█ █▀▀█ ▀▀█▀▀ ▀▀█▀▀ █░░ █▀▀ █▀▀ █░░█ ░▀░ █▀▀█\n", "▒█▀▀▄ █▄▄█ ░░█░░ ░░█░░ █░░ █▀▀ ▀▀█ █▀▀█ ▀█▀ █░░█\n", "▒█▄▄█ ▀░░▀ ░░▀░░ ░░▀░░ ▀▀▀ ▀▀▀ ▀▀▀ ▀░░▀ ▀▀▀ █▀▀▀\n"};
	for (int i = 0; i < 3; i++){

		mvwprintw(master, 3+i, (COLS/2)-(title_len/2), title[i]);
	}
	box(master, 0, 0);
	wattron(master, A_BLINK);
	int msg_len = 29;
	mvwprintw(master, 8, COLS/2-msg_len/2, "Press any button to continue");	
	wattroff(master, A_BLINK);
	wrefresh(master);
}

void setup_field(){

	playerfield = derwin(gamewin, 21, 41, 2, 4);
	enemyfield = derwin(gamewin, 21,41, 2, COLS-49);
	playerfleet = derwin(gamewin, 21, 21, 2, 48);
	enemyfleet = derwin(gamewin, 21, 21, 2, COLS-73);
	refresh();

	box(playerfield, 0, 0);
	mvwprintw(playerfield, 0, 3, " Your Battlefield ");	
	for(int i = 0; i < 10; i++) mvwprintw(gamewin, 1, 6+(i*4), "%c", 'A'+i);
	for(int i = 0; i < 10; i++) mvwprintw(gamewin, 3+(i*2), 2, "%c", '0'+i);	
	for(int i = 1; i < 10; i++) mvwhline(playerfield, i*2, 1, 0, 39);
	for(int i = 1; i < 10; i++) mvwvline(playerfield, 1, i*4, 0, 19);
	box(playerfleet, 0, 0);
	mvwprintw(playerfleet, 0, 3, " Your Fleet ");
	
	mvwprintw(playerfleet, 2, 2, "Carrier"); 
	mvwprintw(playerfleet, 3, 2, "<= === === === =>");
	mvwprintw(playerfleet, 5, 2, "Battleship"); 
	mvwprintw(playerfleet, 6, 2, "<= === === =>");
	mvwprintw(playerfleet, 8, 2, "Cruiser"); 
	mvwprintw(playerfleet, 9, 2, "<= === =>");
	mvwprintw(playerfleet, 11, 2, "Submarine"); 
	mvwprintw(playerfleet, 12, 2, "<= === =>");
	mvwprintw(playerfleet, 14, 2, "Destroyer"); 
	mvwprintw(playerfleet, 15, 2, "<= =>");

	box(enemyfield, 0, 0);
	mvwprintw(enemyfield, 0, 3, " Enemy Battlefield ");
	for(int i = 0; i < 10; i++) mvwprintw(gamewin, 1, COLS-47+(i*4), "%c", 'A'+i);
	for(int i = 0; i < 10; i++) mvwprintw(gamewin, 3+(i*2), COLS-51, "%c", '0'+i);	
	for(int i = 1; i < 10; i++) mvwhline(enemyfield, i*2, 1, 0, 39);
	for(int i = 1; i < 10; i++) mvwvline(enemyfield, 1, i*4, 0, 19);
	box(enemyfleet, 0, 0);
	mvwprintw(enemyfleet, 0, 3, " Enemy Fleet ");

	mvwprintw(enemyfleet, 2, 2, "Carrier"); 
	mvwprintw(enemyfleet, 3, 2, "<= === === === =>");
	mvwprintw(enemyfleet, 5, 2, "Battleship"); 
	mvwprintw(enemyfleet, 6, 2, "<= === === =>");
	mvwprintw(enemyfleet, 8, 2, "Cruiser"); 
	mvwprintw(enemyfleet, 9, 2, "<= === =>");
	mvwprintw(enemyfleet, 11, 2, "Submarine"); 
	mvwprintw(enemyfleet, 12, 2, "<= === =>");
	mvwprintw(enemyfleet, 14, 2, "Destroyer"); 
	mvwprintw(enemyfleet, 15, 2, "<= =>");

	wrefresh(gamewin);
	wrefresh(playerfield);
	wrefresh(enemyfield);
	wrefresh(playerfleet);
	wrefresh(enemyfleet);
}

void single_player(){

	gamewin = derwin(master, LINES-(LINES/3)-1, COLS-4, LINES/3, 2); 
	refresh();
	box(gamewin, 0, 0);
	wrefresh(gamewin);

	setup_field();

	getch();

}

void multi_player(){

	

}

void main_menu(){
	
	int msg_len = 29;
	for(int i = 0; i < 29; i++) mvwprintw(master, 8, (COLS/2 - msg_len/2)+i, " ");
	wrefresh(master);
	
	int win_len = 50;
	WINDOW *main = malloc(sizeof(WINDOW));
	main = derwin(master, 10, win_len, LINES/2, COLS/2-win_len/2);
	refresh();
	keypad(main, true);

	int cursor = 0;
	int key;
	do{
		wclear(main);
		box(main, 0, 0);	
		mvwprintw(main, 0, 3, " Main Menu ");
		mvwprintw(main, 9, 15, " Scroll:[UP, DOWN] Select:[SPACE] ");

		if(cursor > 2) cursor = 2;
		if(cursor < 0) cursor = 0;

		char menu[3][16] = {"- Single Player", "- Multi Player ", "- Exit         "};
		for(int i = 0; i < 3; i++){
			if (i == cursor) wattron(main, A_REVERSE);
			mvwprintw(main, 2+(2*i), 6, menu[i]);
			wattroff(main, A_REVERSE);
		}	
		
		wattroff(main, A_REVERSE);
		wrefresh(main);

		key = wgetch(main);
	
		if(key == KEY_UP) cursor--;
		if(key == KEY_DOWN) cursor++;

	} while (key != 10);
	
	switch(cursor){
		case 0:
			werase(main);
			single_player();
			break;
		case 1:
			multi_player();
			break;
		case 2:
			curs_set(1);
			endwin();
			exit(0);
			break;
	}
}

int main(int argc, char **argv){
		
	init_curses();

	curs_set(0);
	print_title_screen();
	getch();
	
	main_menu();	
	
	curs_set(1);
	endwin();

	return 0;

}

