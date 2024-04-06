#include <stdlib.h>
#include <curses.h>
#include <locale.h>
#include <time.h>

#define cds(y, x)((10*(y))+x)

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
	char title[3][128] ={"▒█▀▀█ █▀▀█ ▀▀█▀▀ ▀▀█▀▀ █░░ █▀▀ █▀▀ █░░█ ░▀░ █▀▀█\n","▒█▀▀▄ █▄▄█ ░░█░░ ░░█░░ █░░ █▀▀ ▀▀█ █▀▀█ ▀█▀ █░░█\n","▒█▄▄█ ▀░░▀ ░░▀░░ ░░▀░░ ▀▀▀ ▀▀▀ ▀▀▀ ▀░░▀ ▀▀▀ █▀▀▀\n"};
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

void print_field(WINDOW *field, int y, int x, char *title){

	box(field, 0, 0);
	mvwprintw(field, 0, 3, title);	
	for(int i = 0; i < 10; i++) mvwprintw(gamewin, y-1, x+2+(i*4), "%c", 'A'+i);
	for(int i = 0; i < 10; i++) mvwprintw(gamewin, y+1+(i*2), x-2, "%c", '0'+i);	
	for(int i = 1; i < 10; i++) mvwhline(field, i*2, 1, 0, 39);
	for(int i = 1; i < 10; i++) mvwvline(field, 1, i*4, 0, 19);
}

void print_fleet(WINDOW *win, char *title){

	box(win, 0, 0);
	mvwprintw(win, 0, 3, title);
	mvwprintw(win, 2, 2, "Carrier"); 
	mvwprintw(win, 3, 2, "<= === === === =>");
	mvwprintw(win, 5, 2, "Battleship"); 
	mvwprintw(win, 6, 2, "<= === === =>");
	mvwprintw(win, 8, 2, "Cruiser"); 
	mvwprintw(win, 9, 2, "<= === =>");
	mvwprintw(win, 11, 2, "Submarine"); 
	mvwprintw(win, 12, 2, "<= === =>");
	mvwprintw(win, 14, 2, "Destroyer"); 
	mvwprintw(win, 15, 2, "<= =>");
}

void setup_fields(){

	playerfield = derwin(gamewin, 21, 41, 2, 4);
	enemyfield = derwin(gamewin, 21,41, 2, COLS-49);
	playerfleet = derwin(gamewin, 21, 21, 2, 48);
	enemyfleet = derwin(gamewin, 21, 21, 2, COLS-73);
	refresh();

	print_field(playerfield, 2, 4, " Your Battlefield ");
	print_fleet(playerfleet, " Your Fleet ");
	
	print_field(enemyfield, 2, COLS-49, " Enemy BattleField ");
	print_fleet(enemyfleet, " Enemy Fleet ");

	wrefresh(gamewin);
	wrefresh(playerfield);
	wrefresh(enemyfield);
	wrefresh(playerfleet);
	wrefresh(enemyfleet);
}

void draw_map(WINDOW *field, char *map, bool mode){

	for (int i = 0; i < 10; i++){
		for (int j = 0; j < 10; j++){
			switch (map[cds(i,j)]) {
				case 1: 
					if (mode) mvwprintw(field, 1+i*2, 1+j*4, " <═");
					break;
				case 2: 
					if (mode) mvwprintw(field, 1+i*2, 1+j*4, "═══");
					break;
				case 3: 
					if (mode) mvwprintw(field, 1+i*2, 1+j*4, "═> ");
					break;
				case 4:
					if (mode) mvwprintw(field, 1+i*2, 1+j*4, " ^ ");
					break;
				case 5:
					if (mode) mvwprintw(field, 1+i*2, 1+j*4, " ║ ");
					break;
				case 6:
					if (mode) mvwprintw(field, 1+i*2, 1+j*4, " v ");
					break;
				case 7:
					mvwprintw(field, 1+i*2, 2+j*4, "X");
					break;
				case 8:
					mvwprintw(field, 1+i*2, 2+j*4, "O");
					break;
			}
		}
	}
}

char *position_fleet(){

	keypad(playerfield, true);

	int rem_ships = 4;
	char *player_map = calloc(100, sizeof(char));
	int x = 0;
	int y = 0;
	bool rot = true;
	int ship[5] = {1, 2, 2, 3, 4};
	
	do{
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (rot){ 
			if (y > 9) y = 9;
			if (x+ship[rem_ships] > 9) x = 9-ship[rem_ships];
		} else {
			if (x > 9) x = 9;
			if (y+ship[rem_ships] > 9) y = 9-ship[rem_ships];
		}
		werase(playerfield);
		print_field(playerfield, 2, 4, " Your Battlefield ");
		draw_map(playerfield, player_map, true);		
		
		wattron(playerfield, A_BLINK);
		if(rot){
			mvwprintw(playerfield, 1+y*2, 1+x*4, " <═");
			for (int i = 1; i < ship[rem_ships]; i++) mvwprintw(playerfield, 1+y*2, 1+(i+x)*4, "═══");
			mvwprintw(playerfield, 1+y*2, 1+(ship[rem_ships]+x)*4, "═> ");
		}
		else{
			mvwprintw(playerfield, 1+y*2, 1+x*4, " ^ ");
			for (int i = 1; i < ship[rem_ships]; i++) mvwprintw(playerfield, 1+(i+y)*2, 1+x*4, " ║ ");
			mvwprintw(playerfield, 1+(ship[rem_ships]+y)*2, 1+x*4, " v ");
		}
		wattroff(playerfield, A_BLINK);

		wrefresh(playerfield);
		
		int key = wgetch(playerfield);
		if (key == KEY_UP) y--;
		if (key == KEY_DOWN) y++;
		if (key == KEY_LEFT) x--;
		if (key == KEY_RIGHT) x++;
		if (key == ' ') {
			if (rot){
				if (y+ship[rem_ships] <= 9) rot = !rot;
			}else{
				if (x+ship[rem_ships] <= 9) rot = !rot;
			}
		}
		if (key == 10) {
			if (rot){
				player_map[cds(y,x)] = 1;
				for (int i = 1; i < ship[rem_ships]; i++) player_map[cds(y,x+i)] = 2;
				player_map[cds(y,x+ship[rem_ships])] = 3;
			}
			else{
				player_map[cds(y,x)] = 4;
				for (int i = 1; i < ship[rem_ships]; i++) player_map[cds(y+i,x)] = 5;
				player_map[cds(y+ship[rem_ships], x)] = 6;

			}
			rem_ships--;
		}
	} while(rem_ships >= 0);

	keypad(playerfield, false);
	werase(playerfield);
	print_field(playerfield, 2, 4, " Your Battlefield ");
	draw_map(playerfield, player_map, true);
	wrefresh(playerfield);

	return player_map;

}

char *setup_enemy(){

	char *enemy_map = calloc(100, sizeof(char));

	int rem_ships = 4;
	int ship[5] = {1, 2, 2, 3, 4};
	
	srand(time(NULL));

	do{
		int x;
		int y;
		bool rot = 0;
		bool placed;

		do{
			placed = true;
			rot = rand() % 2;
			if (rot){
 				x = rand() % (10-ship[rem_ships]);
				y = rand() % 10;
			} else {
				x = rand() % 10;
				y = rand() % (10-ship[rem_ships]);
			}
			for(int i = 0; i < ship[rem_ships]+1; i++){
				if (rot){
					if (y > 10 || x+i > 10 || enemy_map[cds(y,x+i)] != 0) placed = false;
				} else {
					if (y+i > 10 || x > 10 || enemy_map[cds(y+i,x)] != 0) placed = false;
				}
			}
		} while (!placed);
		if (rot){
			enemy_map[cds(y,x)] = 1;
			for (int i = 1; i < ship[rem_ships]; i++) enemy_map[cds(y,x+i)] = 2;
			enemy_map[cds(y,x+ship[rem_ships])] = 3;
		}else{
			enemy_map[cds(y,x)] = 4;
			for (int i = 1; i < ship[rem_ships]; i++) enemy_map[cds(y+i,x)] = 5;
			enemy_map[cds(y+ship[rem_ships],x)] = 6;
		}
		rem_ships --;

	} while (rem_ships >= 0);
	
	return enemy_map;
}

void single_player(){

	gamewin = derwin(master, LINES-(LINES/3)-1, COLS-4, LINES/3, 2); 
	refresh();
	box(gamewin, 0, 0);
	wrefresh(gamewin);

	setup_fields();
	char *player_map = position_fleet();
	char *enemy_map = setup_enemy();

	bool turn = true;

	int x = 0;
	int y = 0;
	int key;
	
	keypad(enemyfield, true);
	do{
		if (turn){

			if(x > 9) x = 9;
			if(x < 0) x = 0;
			if(y > 9) y = 9;
			if(y < 0) y = 0;

			werase(enemyfield);
			print_field(enemyfield, 2, COLS-49, " Enemy Battlefield ");
			draw_map(enemyfield, enemy_map, false);
	
			wattron(enemyfield, A_BLINK);
			mvwprintw(enemyfield, 1+y*2, 2+x*4, "⨀");
			wattroff(enemyfield, A_BLINK);
	
			key = wgetch(enemyfield);
			if (key == KEY_UP) y--;
			if (key == KEY_DOWN) y++;
			if (key == KEY_LEFT) x--;
			if (key == KEY_RIGHT) x++;
			if (key == 10){
				if(enemy_map[cds(y,x)] != 0){
					enemy_map[cds(y,x)] = 7;
				} else {
					enemy_map[cds(y,x)] = 8;
				}
				turn = false;
			}
			wrefresh(enemyfield);
		} else {
			int ex, ey;
			//do{
			ex = rand() % 10;
			ey = rand() % 10;
			//} while(player_map[cds(ey,ex)] < 7);
			
			if(player_map[cds(ey,ex)] != 0 && player_map[cds(ey,ex)] != 8){
				player_map[cds(ey,ex)] = 7;
			} else {
				player_map[cds(ey,ex)] = 8;
			}
			turn = true;
			werase(playerfield);
			print_field(playerfield, 2, 4, " Your Battlefield ");
			draw_map(playerfield, player_map, true);
			wrefresh(playerfield);	
		}

	}while(key != 27);

	keypad(enemyfield, false);

	werase(playerfield);
	werase(playerfleet);
	werase(enemyfield);
	werase(enemyfleet);
	werase(gamewin);
	wrefresh(playerfield);
	wrefresh(playerfleet);
	wrefresh(enemyfield);
	wrefresh(enemyfleet);
	wrefresh(gamewin);
	refresh();
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
	bool quit = false;
	do{
		do{
			werase(main);
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
				quit = true;
				break;
		}
	}while(!quit);
	keypad(main, false);
}

int main(int argc, char **argv){
		
	init_curses();

	curs_set(0);
	print_title_screen();
	getch();
	
	main_menu();	
	
	fflush(stdin);
	fflush(stdout);
	curs_set(1);
	endwin();

	return 0;

}

