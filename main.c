#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <locale.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>

#define cds(y, x)((10*(y))+x)

WINDOW *master;
typedef struct game_win{
    	WINDOW *win;
    	WINDOW *radiolog;
    	WINDOW *playerfield;
    	WINDOW *enemyfield;
    	WINDOW *playerfleet;
    	WINDOW *enemyfleet;
    	char *player_map;
    	char *enemy_map;
}game_win;

typedef struct server_t {
	char *name;
	char *ip;
	int port;
} server_t;

bool CONNECTED = false;
bool READY = false;
bool ERROR = false;
int CLIENT, SERVER;
char *ADDRESS;
int PORT;
bool turn = true;
int mx, my, line = 1;

void init_curses(){
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	master = newwin(0,0,0,0);
	refresh();
}

void print_title_screen(WINDOW *win, bool mode){
	
	int title_len = 48;
	char title[3][128] ={"▒█▀▀█ █▀▀█ ▀▀█▀▀ ▀▀█▀▀ █░░ █▀▀ █▀▀ █░░█ ░▀░ █▀▀█\n","▒█▀▀▄ █▄▄█ ░░█░░ ░░█░░ █░░ █▀▀ ▀▀█ █▀▀█ ▀█▀ █░░█\n","▒█▄▄█ ▀░░▀ ░░▀░░ ░░▀░░ ▀▀▀ ▀▀▀ ▀▀▀ ▀░░▀ ▀▀▀ █▀▀▀\n"};
	for (int i = 0; i < 3; i++) mvwprintw(win, ((LINES/3)/2)-1+i, (COLS/2)-(title_len/2), title[i]);
	box(win, 0, 0);
    	if (mode) {
        	wattron(win, A_BLINK);
        	mvwprintw(win, ((LINES / 3) / 2) + 3, COLS / 2 - 29 / 2, "Press any button to continue");
        	wattroff(win, A_BLINK);
    	}
	wrefresh(win);
}

void print_field(WINDOW *field, WINDOW *win, int y, int x, char *title){

	box(field, 0, 0);
	mvwprintw(field, 0, 3, title);	
	for(int i = 0; i < 10; i++) mvwprintw(win, y-1, x+2+(i*4), "%c", 'A'+i);
	for(int i = 0; i < 10; i++) mvwprintw(win, y+1+(i*2), x-2, "%c", '0'+i);
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

void setup_fields(game_win *gamewin){

    	gamewin->playerfield = derwin(gamewin->win, 21, 41, 2, 4);
    	gamewin->enemyfield = derwin(gamewin->win, 21, 41, 2, 93);
    	gamewin->playerfleet = derwin(gamewin->win, 21, 21, 2, 48);
    	gamewin->enemyfleet = derwin(gamewin->win, 21, 21, 2, 69);
    	gamewin->radiolog = derwin(gamewin->win, 7, 130, 23, 4);
	refresh();

	print_field(gamewin->playerfield, gamewin->win, 2, 4, " Your Battlefield ");
	print_fleet(gamewin->playerfleet, " Your Fleet ");
	
	print_field(gamewin->enemyfield, gamewin->win, 2, 93, " Enemy BattleField ");
	print_fleet(gamewin->enemyfleet, " Enemy Fleet ");

	box(gamewin->radiolog, 0, 0);
	mvwprintw(gamewin->radiolog, 0, 3, " Radio Log ");
	
	wrefresh(gamewin->playerfield);
	wrefresh(gamewin->enemyfield);
	wrefresh(gamewin->playerfleet);
	wrefresh(gamewin->enemyfleet);
	wrefresh(gamewin->radiolog);
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

void position_fleet(game_win *gamewin, bool mp){

	keypad(gamewin->playerfield, true);

	int rem_ships = 4;
	gamewin->player_map = calloc(100, sizeof(char));
	int x = 0;
	int y = 0;
	bool rot = true;
	int ship[5] = {1, 2, 2, 3, 4};
	wtimeout(gamewin->playerfield, 5);
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

		werase(gamewin->playerfield);
		print_field(gamewin->playerfield, gamewin->win, 2, 4, " Your Battlefield ");
		draw_map(gamewin->playerfield, gamewin->player_map, true);
		
		wattron(gamewin->playerfield, A_BLINK);
		if(rot){
			mvwprintw(gamewin->playerfield, 1+y*2, 1+x*4, " <═");
			for (int i = 1; i < ship[rem_ships]; i++) mvwprintw(gamewin->playerfield, 1+y*2, 1+(i+x)*4, "═══");
			mvwprintw(gamewin->playerfield, 1+y*2, 1+(ship[rem_ships]+x)*4, "═> ");
		}
		else{
			mvwprintw(gamewin->playerfield, 1+y*2, 1+x*4, " ^ ");
			for (int i = 1; i < ship[rem_ships]; i++) mvwprintw(gamewin->playerfield, 1+(i+y)*2, 1+x*4, " ║ ");
			mvwprintw(gamewin->playerfield, 1+(ship[rem_ships]+y)*2, 1+x*4, " v ");
		}
		wattroff(gamewin->playerfield, A_BLINK);

		wrefresh(gamewin->playerfield);
		
		int key = wgetch(gamewin->playerfield);
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
			bool placed = true;
			if (rot){
				for(int i = 0; i < ship[rem_ships]+1; i++) if (gamewin->player_map[cds(y,x+i)] != 0) placed = false;
			        if (placed){	
					gamewin->player_map[cds(y,x)] = 1;
					for (int i = 1; i < ship[rem_ships]; i++) gamewin->player_map[cds(y,x+i)] = 2;
					gamewin->player_map[cds(y,x+ship[rem_ships])] = 3;
				}
			}
			else{
				for(int i = 0; i < ship[rem_ships]+1; i++) if (gamewin->player_map[cds(y+i,x)] != 0) placed = false;
			        if (placed){
					gamewin->player_map[cds(y,x)] = 4;
					for (int i = 1; i < ship[rem_ships]; i++) gamewin->player_map[cds(y+i,x)] = 5;
					gamewin->player_map[cds(y+ship[rem_ships], x)] = 6;
				}
			}
			if (placed) rem_ships--;
		}
		if (key == 27 || (!CONNECTED && mp)){
			*gamewin->player_map = 10;
			break;
		}
	} while(rem_ships >= 0);
	
	wtimeout(gamewin->playerfield, -1);

	keypad(gamewin->playerfield, false);
	werase(gamewin->playerfield);
	print_field(gamewin->playerfield, gamewin->win, 2, 4, " Your Battlefield ");
	draw_map(gamewin->playerfield, gamewin->player_map, true);
	wrefresh(gamewin->playerfield);

}

char *setup_enemy(){

	char *map = calloc(100, sizeof(char));

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
					if (y > 10 || x+i > 10 || map[cds(y,x+i)] != 0) placed = false;
				} else {
					if (y+i > 10 || x > 10 || map[cds(y+i,x)] != 0) placed = false;
				}
			}
		} while (!placed);
		if (rot){
			map[cds(y,x)] = 1;
			for (int i = 1; i < ship[rem_ships]; i++) map[cds(y,x+i)] = 2;
			map[cds(y,x+ship[rem_ships])] = 3;
		}else{
			map[cds(y,x)] = 4;
			for (int i = 1; i < ship[rem_ships]; i++) map[cds(y+i,x)] = 5;
			map[cds(y+ship[rem_ships],x)] = 6;
		}
		rem_ships --;

	} while (rem_ships >= 0);
	
	return map;
}

void game_loop(game_win *gamewin, bool mp){

    	position_fleet(gamewin, mp);

	if(*gamewin->player_map != 10){

		if (mp) {
			write(CLIENT,"READY", 6);
			while(!READY) sleep(1);
            		gamewin->enemy_map = calloc(100, sizeof(char));
			wtimeout(gamewin->enemyfield, 5);
		}
		else gamewin->enemy_map = setup_enemy();

		int x = 0;
		int y = 0;
		int key;
	
		keypad(gamewin->enemyfield, true);
		do{
			if (turn){
	
				if(x > 9) x = 9;
				if(x < 0) x = 0;
				if(y > 9) y = 9;
				if(y < 0) y = 0;
	
				werase(gamewin->enemyfield);
				print_field(gamewin->enemyfield, gamewin->win, 2, 93, " Enemy BattleField ");
				draw_map(gamewin->enemyfield, gamewin->enemy_map, false);
		
				wattron(gamewin->enemyfield, A_BLINK);
				mvwprintw(gamewin->enemyfield, 1+y*2, 2+x*4, "⨀");
				wattroff(gamewin->enemyfield, A_BLINK);
		
				key = wgetch(gamewin->enemyfield);
				if (key == KEY_UP) y--;
				if (key == KEY_DOWN) y++;
				if (key == KEY_LEFT) x--;
				if (key == KEY_RIGHT) x++;
				if (key == 10){
					if (mp){
						char msg[4] = {'F', '0'+y, '0'+x, '\0'};
						mx = x;
						my = y;
						write(CLIENT,(char*)msg, 4);
					} else {
						if(gamewin->enemy_map[cds(y,x)] != 0 && gamewin->enemy_map[cds(y,x)] != 8){
							gamewin->enemy_map[cds(y,x)] = 7;
						} else {
							gamewin->enemy_map[cds(y,x)] = 8;
						}
					}
					wrefresh(gamewin->radiolog);
					turn = false;
				}
				wrefresh(gamewin->enemyfield);
			} else {
				if (mp){
					while(!turn && CONNECTED && key != 27) {
						key = wgetch(gamewin->enemyfield);
						sleep(1);
					}
				} else {
					int ex, ey;
					
					ex = rand() % 10;
					ey = rand() % 10;
						
					if(gamewin->player_map[cds(ey,ex)] != 0 && gamewin->player_map[cds(ey,ex)] != 8){
                        			gamewin->player_map[cds(ey,ex)] = 7;
					} else {
                        			gamewin->player_map[cds(ey,ex)] = 8;
					}
					turn = true;
					werase(gamewin->playerfield);
					print_field(gamewin->playerfield, gamewin->win, 2, 4, " Your Battlefield ");
					draw_map(gamewin->playerfield, gamewin->player_map, true);
					wrefresh(gamewin->playerfield);
				}
			}
	
			if(mp && !CONNECTED) break;
		}while(key != 27);
		
		keypad(gamewin->enemyfield, false);
	
		free(gamewin->enemy_map);
	}
	if(mp){
		write(CLIENT, "DISC", 4);
		wtimeout(gamewin->enemyfield, -1);
		CONNECTED = false;
	}
	free(gamewin->player_map);

}

game_win *create_gamewin(){
    	game_win *gamewin = calloc(1, sizeof(game_win));
	gamewin->win = derwin(master, 31, 138, LINES/3, (COLS/2)-(138/2));
	refresh();
	box(gamewin->win, 0, 0);
	mvwprintw(gamewin->win, 30, 66, " Move:[UP,DOWN,LEFT,RIGHT] Place Ship/Fire:[ENTER] Rotate Ship:[SPACE] ");
	setup_fields(gamewin);
	wrefresh(gamewin->win);
    	return gamewin;
}

void close_gamewin(game_win *gamewin){
	werase(gamewin->playerfield);
	werase(gamewin->playerfleet);
	werase(gamewin->enemyfield);
	werase(gamewin->enemyfleet);
	werase(gamewin->radiolog);
	werase(gamewin->win);
	wrefresh(gamewin->playerfield);
	wrefresh(gamewin->playerfleet);
	wrefresh(gamewin->enemyfield);
	wrefresh(gamewin->enemyfleet);
	wrefresh(gamewin->radiolog);
	wrefresh(gamewin->win);
    	delwin(gamewin->playerfield);
    	delwin(gamewin->playerfleet);
    	delwin(gamewin->enemyfield);
    	delwin(gamewin->enemyfleet);
    	delwin(gamewin->radiolog);
    	delwin(gamewin->win);
	refresh();
    	free(gamewin);
}

void error(const char *msg){
	WINDOW *errwin = newwin(10, 40, (LINES/2)-5, (COLS/2)-20);
	refresh();
	box(errwin, 0, 0);
	mvwprintw(errwin, 0, 3, " !ERROR! ");
	mvwprintw(errwin, 4, 2, "%s", msg);
	wrefresh(errwin);
	wgetch(errwin);
	werase(errwin);
	wrefresh(errwin);
	delwin(errwin);
	refresh();
}

void getparse_msg(game_win *gamewin){

	int n;
	char *buffer;	

	while(CONNECTED){	
	
		buffer = calloc(256, sizeof(char));
		
		// Read form socket
		n = read(CLIENT,buffer,255);
		if (n < 0) {
			error("ERROR reading from socket");
			pthread_exit((void *)-1);
		}

		// Print to screen log
		if (CONNECTED){			
			int rly, rlx;
			getyx(gamewin->radiolog, rly, rlx);
			if (rlx > 127-n){
				line ++;
			       	wmove(gamewin->radiolog, line, 3);
				wrefresh(gamewin->radiolog);
			}
			wprintw(gamewin->radiolog, "%s ", buffer);
			wrefresh(gamewin->radiolog);
		}
	
		// Parse message and act on it 
		if (strcmp(buffer, "READY") == 0){
			READY = true; // Set ready flag
		}
		if (strcmp(buffer, "HIT") == 0){
            		gamewin->enemy_map[cds(my, mx)] = 7; // Set enemy map to hit marker
			// Redraw enemy field
			werase(gamewin->enemyfield);
			print_field(gamewin->enemyfield, gamewin->win, 2, 93, " Enemy BattleField ");
			draw_map(gamewin->enemyfield, gamewin->enemy_map, false);
			wrefresh(gamewin->enemyfield);
		}
		if (strcmp(buffer, "MISS") == 0){
            		gamewin->enemy_map[cds(my, mx)] = 8; // Set enemy map to miss marker
			// Redraw enemy field
			werase(gamewin->enemyfield);
			print_field(gamewin->enemyfield, gamewin->win, 2, 93, " Enemy BattleField ");
			draw_map(gamewin->enemyfield, gamewin->enemy_map, false);
			wrefresh(gamewin->enemyfield);
		}
		if (buffer[0] == 'F'){ // Enemy fire message format : F[y][x] 
			// Convert coordinates to int 
			int y = buffer[1]-'0';
			int x = buffer[2]-'0';
			if (gamewin->player_map[cds(y,x)] != 0 && gamewin->player_map[cds(y,x)] != 8){
				// If there is a player ship
                		gamewin->player_map[cds(y,x)] = 7; // Set player map to hit marker
				n = write(CLIENT, "HIT", 3); // Send back hit message
			} else {
				// If the re isn't a player ship
                		gamewin->player_map[cds(y,x)] = 8; // Set player map to miss marker
				n = write(CLIENT, "MISS", 4); // Send back miss message
			}
			// Redraw player field
			werase(gamewin->playerfield);
			print_field(gamewin->playerfield, gamewin->win, 2, 4, " Your Battlefield ");
			draw_map(gamewin->playerfield, gamewin->player_map, true);
			wrefresh(gamewin->playerfield);
			// Change turn
			turn = true;
		}
		if (strcmp(buffer, "DISC") == 0){ // Enemy disconnected
			CONNECTED = false;
			return;
		}

		free(buffer);
	}
}

void *init_host(void *gamewin){
	
	// Set thread cancel type to asynchronus to allow for instant cancelation on cancel signal reception 
	int oldt;	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldt);	
	
	// Allocate space for socket server variables and structs
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	// Open socket
	SERVER = socket(AF_INET, SOCK_STREAM, 0);
	if (SERVER < 0) {
		error("ERROR opening socket");
        	ERROR = true;
		pthread_exit((void *)-1);
	}
	
	// Populate socket server structs
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	// Bind socket address to server 
	if (bind(SERVER, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on binding");
        	ERROR = true;
        	close(SERVER);
		pthread_exit((void *)-1);
	}
	
	// Listen for client connection and accept connection
	listen(SERVER,5);
	clilen = sizeof(cli_addr);
	CLIENT = accept(SERVER, (struct sockaddr *) &cli_addr, &clilen);

	if (CLIENT < 0){
        	error("ERROR on accept");
        	ERROR = true;
		pthread_exit((void *)-1);
	}
	
	CONNECTED = true;
	
	// Start parseing message
	getparse_msg((game_win*)gamewin);

	close(SERVER);
	close(CLIENT);

	pthread_exit((void *)1);
}

void *init_client(void *gamewin){
	
	// Set thread cancel type to asynchronus to allow for instant cancelation on cancel signal reception 
	int oldt;	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldt);	

	// Allocate space for socket client structs
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	// Open socket and get host
	CLIENT = socket(AF_INET, SOCK_STREAM, 0);
	server = gethostbyname(ADDRESS);

	if (server == NULL) {
		error("ERROR, no such host");
        	ERROR = true;
		pthread_exit((void *)1);
	}
	
	// Populate socket client structs
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(PORT);
	
	// Connect to host
	if (connect(CLIENT,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		error("ERROR connecting");
        	ERROR = true;
        	close(CLIENT);
		pthread_exit((void *)1);
	}
	
	CONNECTED = true;

	// Start parseing messages
	getparse_msg((game_win*)gamewin);

	close(CLIENT);
	
	pthread_exit((void *)1);
} 

void multiplayer(bool mode){
	
	/* bool mode: Flag true for client, false for host */

	// Set ERROR flag and create game window
    	ERROR = false;
    	game_win *gamewin = create_gamewin();

	// Create socket thread
	pthread_t tid;
	pthread_create(&tid, NULL, mode ? init_client : init_host, (void *)gamewin);
	
	// Print to screen log
	wmove(gamewin->radiolog, 1, 3);
	wprintw(gamewin->radiolog, mode ? "Connecting to host... " : "Waiting for player... ");
	wrefresh(gamewin->radiolog);

	// Wait for connection to host, error or exit signal
	wtimeout(gamewin->enemyfield, 5);
	while(!CONNECTED && !ERROR){
		int key = wgetch(gamewin->enemyfield);
		if (key == 27){ // if ESC is pressed, exit
			wtimeout(gamewin->enemyfield, -1);
			close_gamewin(gamewin);
			if(mode) close(CLIENT);
			else close(SERVER);
			pthread_cancel(tid);
			return;
		}
		sleep(1);
	}
    	wtimeout(gamewin->enemyfield, -1);
	
	// Connection successful
	if (!ERROR) {
        	wprintw(gamewin->radiolog, mode ? "Connected to host. " : "Player connected. ");
        	wrefresh(gamewin->radiolog);
		// Start game
        	if(mode) turn = false;
        	game_loop(gamewin, true);
    	}
	close_gamewin(gamewin);
}

void print_main_menu(WINDOW* win, int mp, int c){
	
	werase(win);
	box(win, 0, 0);	
	char *title[3] = {" Main Menu ", " Multi Player ", " Join Game "};
	mvwprintw(win, 0, 3, "%s", title[mp]);
	mvwprintw(win, 9, 15, " Scroll:[UP, DOWN] Select:[ENTER] ");
    
	char *mmenu[3] = {"- Single Player", "- Multi Player", "- Exit"};
	char *mpmenu[3] = {"- Host Classic Game", "- Join Game", "- Back"};
	char *jmenu[3] = {"- Classic", "- Multiuser", "- Back"};

	char **menu;
	switch(mp){
		case 0:
			menu = mmenu;
			break;
		case 1:
			menu = mpmenu;
			break;
		case 2:
			menu = jmenu;
			break;
	}

	for(int i = 0; i < 3; i++){
		if (i == c) wattron(win, A_REVERSE);
		mvwprintw(win, 2+(2*i), 6, menu[i]);
		wattroff(win, A_REVERSE);
	}
    
	wrefresh(win);
}

void mph_menu(WINDOW *win){
	
	// Print menu entrys
	werase(win);
	box(win, 0, 0);	
	mvwprintw(win, 0, 3, " Multi Player ");
	mvwprintw(win, 4, 6, "- Port: ");
	wrefresh(win);

	// Allocate space for port string
	char *p = calloc(6, sizeof(char));
	char *c = p;

	int key;
	do{
		key = wgetch(win);
		if (key == 263){ // Backspace
			int y, x;
			getyx(win, y, x);
			if (x > 14){
				mvwprintw(win, y, x-1, " ");
				wmove(win, y, x-1);
				*c = '\0';
				c--;
			}
		} else if (key == 10){ // Enter
			curs_set(0);
			PORT= atoi(p);
            		werase(win);
			multiplayer(false);
			break;
		} else if (key >= '0' && key <= '9') { // Handle digits
			wprintw(win, "%c", (char)key);
			*c = (char)key;
			c++;
		}
	} while (key != 27);
}

void mpc_menu(WINDOW *win){

	// Print menu entrys
	werase(win);
	box(win, 0, 0);	
	mvwprintw(win, 0, 3, " Multi Player ");
	mvwprintw(win, 3, 6, "- Address:");
	mvwprintw(win, 5, 6, "- Port:");
	wrefresh(win);

	// Allocate space for IP and port strings
	char *p = calloc(16, sizeof(char));
	char *c = p;
	char *a = calloc(6, sizeof(char));
	char *d = a;

	//Initialize variables and window curosr
	int key;
	bool field = false;
	int y, x;
	getyx(win, y, x);
	wmove(win, y-2, 17);

	do{
		key = wgetch(win);
		if (key == 263){ // Backspace
			getyx(win, y, x);
			if (x > (field ? 14 : 17)){
				mvwprintw(win, y, x-1, " ");
				wmove(win, y, x-1);
				if (field){
					*c = '\0';
					c--;
				} else {
					*d = '\0';
					d--;	
				}
			}
		} else if(key == KEY_UP && field){
			getyx(win, y, x);
			field = false;
			wmove(win, y-2, 17+(d-a));
		} else if(key == KEY_DOWN && !field){
			getyx(win, y, x);
			field = true;
			wmove(win, y+2, 14+(c-p));
		} else if (key == 10){ // Enter
			curs_set(0);
			ADDRESS = a;
			PORT = atoi(p);
           		werase(win);
			multiplayer(true);
			break;
		} else if ((key >= '0' && key <= '9') || key == '.'){ // Handle digits and periods
			wprintw(win, "%c", (char)key);
			if(field){
				*c = (char)key;
				c++;
			} else {
				*d = (char)key;
				d++;
			}
		}
	} while (key != 27);	
}

char *str_ce(char *buff, char *src){

		char *c = src;
		int k = 0;	
		do{
			*(buff+k) = *c;
			k++;
		} while (*++c != 0);
		return buff+k;

}

server_t *add_server(WINDOW *serverwin, server_t *server){
	
	WINDOW *win = derwin(serverwin, 9, 48, 4, 1);
	keypad(win, true);
	refresh();
	
	werase(win);
	
	box(win, 0, 0);
	mvwprintw(win, 0, 3, " Add Server ");
	wrefresh(win);

	mvwprintw(win, 2, 3, "Server name: ");
	mvwprintw(win, 4, 3, "Server ip: ");
	mvwprintw(win, 6, 3, "Server port: ");


	int key;

	char *n = calloc(20, sizeof(char));
	char *a = calloc(16, sizeof(char));
        char *p = calloc(6,  sizeof(char));
	
	char *cn, *ca, *cp;

	if (server == NULL){
		cn = n;
		ca = a;
		cp = p;
	} else {
		
		cn = str_ce(n, server->name);
		ca = str_ce(a, server->ip);
		
		sprintf(p, "%d", server->port);
		cp = p;
		while (*++cp != 0);
		
		mvwprintw(win, 2, 16, "%s", n);
		mvwprintw(win, 4, 14, "%s", a);
		mvwprintw(win, 6, 16, "%s", p);
		
		wrefresh(win);
	}

	int field = 1;
	
	curs_set(1);
	do{

		if (field < 1) field = 1;
		if (field > 3) field = 3;
		
		if (field == 2) wmove(win, field*2, 14+(ca-a));
		else wmove(win, field*2, field == 1 ? 16+(cn-n) : 16+(cp-p)); 

		key = wgetch(win);
		
		if (key == 10){

			if(*n != 0 && *a != 0 && *p != 0){
				*cn = '\0';
				*ca = '\0';
				*cp = '\0';
				server = calloc(1, sizeof(server_t));
				server->name = n;
				server->ip = a;
				server->port = atoi(p);
				break;
			}
		}	
		else if (key == KEY_UP) field--;
		else if (key == KEY_DOWN) field ++;	
		else {
			if (field == 1){
				if (key == 263 && (cn-n) > 0){
					*cn = '\0';
					cn--;
					wmove(win, field*2, 16+(cn-n));
					wprintw(win, " ");
				}
				if (((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) && (cn-n) < 20){	
					wprintw(win, "%c", (char)key);
					*cn = (char)key;
					cn++;
				}
			} else {
				if (key == 263){
					if (field == 2 && ca-a > 0) {
						*ca = '\0';
						ca--;
						wmove(win, field*2, 14+(ca-a));
						wprintw(win, " ");
					} 
					if (field == 3 && cp-p > 0) {
						*cp = '\0';
						cp--;
						wmove(win, field*2, 16+(cp-p));
						wprintw(win, " ");
					}
				}
				if (((key >= '0' && key <= '9') || key == '.') && (field == 2 ? (ca-a < 16) : (cp-p < 6))){
					wprintw(win, "%c", (char)key);
					if (field == 2){
						*ca = (char)key;
						ca++;
					}
					if (field == 3){
						*cp = (char)key;
						cp++;
					}
				}
			}
		}
		wrefresh(win);

	}while(key != 27);

	curs_set(0);
	keypad(win, false);

	werase(win);
	wrefresh(win);
	delwin(win);
	refresh();
	
	return server;
}

char *get_username(){

	WINDOW *win = derwin(master, 3, 22, LINES/2-1, COLS/2-22/2);
	keypad(win, true);
	refresh();

	werase(win);

	box(win, 0, 0);
	mvwprintw(win, 0, 1, " Provide a username ");
	wrefresh(win);

	wmove(win, 1, 3);

	curs_set(1);

	char *un = calloc(20, sizeof(char));
	char *c = un;

	int key;
	do{

		wmove(win, 1, 3+(c-un));
	
		key = wgetch(win);
		
		if (key == 263) { // Backspace
			if (c-un > 0){	
				*c = '\0';
				c--;
				wmove(win, 1, 3+(c-un));
				wprintw(win, " ");
			}
		}
		if (key == 10) {
			*c = '\0';
			break;
		}
		if (key == 27) {
			un = NULL;
			break;
		}
		if (((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) && (c-un) < 20){	
			wprintw(win, "%c", (char)key);
			*c = (char)key;
			c++;
		}

		wrefresh(win);
	
	}while(true);
	
	curs_set(0);
	keypad(win, false);
	werase(win);
	wrefresh(win);
	delwin(win);
	refresh();
	return un;

}

char *check_username(FILE *conf){

	char *un;

	if (conf == NULL){ // File didnt exist
		un = get_username();
		if (un == NULL) { // User exited 
			return un;
		}
		// Creating config file
		conf = fopen("multiuser.conf", "w+");
		fprintf(conf, "%s\n0\n", un);
	} else { // File exists
		un = calloc(20, sizeof(char));
		fscanf(conf, "%s", un);
	}
	
	fclose(conf);

	return un;
}

server_t *get_servers(FILE* conf, int ns){
		
	// Allocate space for the list of server structs
	server_t *sl = calloc(ns, sizeof(server_t));
	for (int i = 0; i < ns; i++){
		
		// Allocat space for the server struct entrys
		char *n = calloc(20, sizeof(char));
		char *a = calloc(16, sizeof(char));
		
		// Get data from file
		fscanf(conf, "%s %s %d", n, a, &sl[i].port);
		
		// Populate corresponding server struct
		sl[i].name = n;
		sl[i].ip = a;
	}

	return sl;

}

void serverlist_menu(){
	
	
	FILE *muconf = fopen("multiuser.conf", "r");

	char *username = check_username(muconf);
	if (username == NULL) return;
	
	muconf = fopen("multiuser.conf", "r");

	// Get the number of server from the config file 
	int numservers = 0;
	fseek(muconf, strlen(username), SEEK_SET);
	fscanf(muconf, "%d", &numservers);

	// If there are servers saved in the config file get them 
	server_t *serverlist = numservers > 0 ? get_servers(muconf, numservers) : NULL;

	fclose(muconf);
	

	WINDOW *serverwin = derwin(master, 20, 50, LINES/3, COLS/2-50/2);
	WINDOW *serverdash = derwin(serverwin, 3, 48, 16, 1); 
	refresh();
	keypad(serverwin, true);

	char *dash_entrys[4] = {"Connect", "Edit", "Delete", "Add"};

	int key;
	int serv_cursor = 0;
	int dash_cursor = 0;
	do{

		if (serv_cursor > numservers-1) serv_cursor = numservers-1;
		if (serv_cursor < 0) serv_cursor = 0;
		if (dash_cursor > 3) dash_cursor = 3;
		if (dash_cursor < 0) dash_cursor = 0;

		werase(serverwin);
		box(serverwin, 0, 0);
		mvwprintw(serverwin, 0, 3, " %s's Servers (%d) ", username, numservers);

		werase(serverdash);
		box(serverdash, 0, 0);
		
		for(int i = 0; i < numservers; i++){
			if (i == serv_cursor) wattron(serverwin, A_REVERSE);
			mvwprintw(serverwin, 2+i, 3, "%s", serverlist[i].name);
			wattroff(serverwin, A_REVERSE);
		}
		
		for(int i = 0; i < 4; i++){
			if (i == dash_cursor) wattron(serverdash, A_REVERSE);
			mvwprintw(serverdash, 1, 2+i*(48/4), "%s", dash_entrys[i]);
			wattroff(serverdash, A_REVERSE);
		}
	
		wrefresh(serverdash);	
		wrefresh(serverwin);
		
		key = wgetch(serverwin);
		
		if (key == KEY_UP) serv_cursor--;
		if (key == KEY_DOWN) serv_cursor++;
		if (key == KEY_RIGHT) dash_cursor++;
		if (key == KEY_LEFT) dash_cursor--;

		if (key == 10){
			if(dash_cursor == 3){
				server_t *server = add_server(serverwin, (server_t *)NULL);
				if (server != NULL){

					muconf = fopen("multiuser.conf", "r+");
						
					numservers++;
					fseek(muconf, strlen(username)+1, SEEK_SET);
					fprintf(muconf, "%d", numservers);
					
					fseek(muconf, 0, SEEK_END);
					fprintf(muconf, "%s %s %d\n", server->name, server->ip, server->port);	
					
					fclose(muconf);

					server_t *new_servers = calloc(numservers, sizeof(server_t));
					for(int i = 0; i < numservers-1; i++) new_servers[i] = serverlist[i];
					if (numservers > 0) free(serverlist);
					
					serverlist = new_servers;
					serverlist[numservers-1].name = server->name;
					serverlist[numservers-1].ip = server->ip;
					serverlist[numservers-1].port = server->port;
					
				}
			}
			if (dash_cursor == 2){
				
				numservers--;
				server_t *new_servers = calloc(numservers, sizeof(server_t));
				muconf = fopen("multiuser.conf", "w");
				fprintf(muconf, "%s\n%d\n", username, numservers);
				for (int i = 0; i < numservers+1; i++) 
					if (i != serv_cursor){
						fprintf(muconf, "%s %s %d\n", serverlist[i].name, serverlist[i].ip, serverlist[i].port);
						if (i < serv_cursor) new_servers[i] = serverlist[i];
						else new_servers[i-1] = serverlist[i];
					}
				fclose(muconf);
				free(serverlist);
				serverlist = new_servers;		
			}
			if (dash_cursor == 1){
				
				server_t *server = add_server(serverwin, &serverlist[serv_cursor]);

				muconf = fopen("multiuser.conf", "r+");
				fscanf(muconf, "%*s %*d");	

				server_t *new_servers = calloc(numservers, sizeof(server_t));
				for(int i = 0; i < numservers; i++) {
					if (i == serv_cursor) {
						new_servers[i].name = server->name;
						new_servers[i].ip = server->ip;
						new_servers[i].port = server->port;
						fprintf(muconf, "\n%s %s %d", server->name, server->ip, server->port); 
					} else {
						new_servers[i] = serverlist[i];
						if (i < serv_cursor) fscanf(muconf, "%*s %*s %*d");
						else fprintf(muconf, "\n%s %s %d", serverlist[i].name, serverlist[i].ip, serverlist[i].port);
					}
				}
				fclose(muconf);
				free(serverlist);
				serverlist = new_servers;
			}
		}

	}while(key != 27);
	
	// Free allocated memory
	for(int i = 0; i < numservers; i++){
		free(serverlist[i].name);
		free(serverlist[i].ip);
	}
	if (numservers > 0) free(serverlist);

	keypad(serverwin, false);
	werase(serverdash);
	werase(serverwin);
	wrefresh(serverdash);
	wrefresh(serverwin);
	delwin(serverdash);
	delwin(serverwin);
	refresh();
}

void main_menu(){

	WINDOW *mainwin = derwin(master, 10, 50, LINES/2, COLS/2-50/2);
	refresh();
	keypad(mainwin, true);

	int cursor = 0;
	int key;
	bool quit = false;
	int page = 0;
	do{
		do{
			if(cursor > 2) cursor = 2;
			if(cursor < 0) cursor = 0;

			print_main_menu(mainwin, page, cursor);

			key = wgetch(mainwin);
		
			if(key == KEY_UP) cursor--;
			if(key == KEY_DOWN) cursor++;
			
		} while (key != 10);
		switch(cursor){
		case 0:	
			switch(page){
			case 0: // Singleplayer entry
                   		werase(mainwin);
				game_win *gamewin = create_gamewin();
				wmove(gamewin->radiolog, 1, 3);
				wrefresh(gamewin->radiolog);
				game_loop(gamewin, false);
				close_gamewin(gamewin);
				break;
			case 1: // 1v1 Classic host submenu
				curs_set(1);
				mph_menu(mainwin);
				curs_set(0);
				break;
			case 2: // Join 1v1 classic submenu 
				curs_set(1);
				mpc_menu(mainwin);
				curs_set(0);
				break;
			}
		break;
		case 1:
			switch(page){
			case 0: // Multiplayer entry
				cursor = 0;
				page = 1;
				break;
			case 1: // Multiplayer join submenu
				cursor = 0;
				page = 2;			
				break;
			case 2: // Join Multiuser skirmish submenu
				werase(mainwin);
				wrefresh(mainwin);
				serverlist_menu();
				break;
			}
		break;
		case 2:
			switch(page){
			case 0:  // Quit entry
				quit = true;
				break;
			case 1: // Multiplayer emnu back entry
				cursor = 0;
				page = 0;
				break;
			case 2:
				cursor = 0;
				page = 1;
				break;
			}
		break;
		}
	}while(!quit);
	keypad(mainwin, false);
}

int main(int argc, char **argv){

    	init_curses();

    	curs_set(0);

    	int y, x;
    	getmaxyx(master, y, x);
    	if (y < 32|| x < 139){
    	    	error("ERROR, terminal window too small.");
    	    	endwin();
    	    	curs_set(1);
    		return 1;
    	}

	print_title_screen(master, true);
	getch();
	werase(master);
    	print_title_screen(master, false);

	main_menu();	
	
	curs_set(1);
	endwin();

	return 0;

}

