#include <stdlib.h>
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

bool CONNECTED = false;
bool READY = false;
int CLIENT, SERVER;
char *ADDRESS;
int PORT;
bool turn = true;
int mx, my;

void init_curses(){
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	master = newwin(0,0,0,0);
	refresh();
}

void print_title_screen(WINDOW *win){
	
	int title_len = 48;
	char title[3][128] ={"▒█▀▀█ █▀▀█ ▀▀█▀▀ ▀▀█▀▀ █░░ █▀▀ █▀▀ █░░█ ░▀░ █▀▀█\n","▒█▀▀▄ █▄▄█ ░░█░░ ░░█░░ █░░ █▀▀ ▀▀█ █▀▀█ ▀█▀ █░░█\n","▒█▄▄█ ▀░░▀ ░░▀░░ ░░▀░░ ▀▀▀ ▀▀▀ ▀▀▀ ▀░░▀ ▀▀▀ █▀▀▀\n"};
	for (int i = 0; i < 3; i++) mvwprintw(win, ((LINES/3)/2)-1+i, (COLS/2)-(title_len/2), title[i]);
	box(win, 0, 0);
	wattron(win, A_BLINK);
	mvwprintw(win, 11, COLS/2-29/2, "Press any button to continue");	
	wattroff(win, A_BLINK);
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
		if (key == 27 || !CONNECTED && mp){
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

		n = read(CLIENT,buffer,255);
		if (CONNECTED){			
			wprintw(gamewin->radiolog, "%s ", buffer);
			wrefresh(gamewin->radiolog);
		}
	
		if (n < 0) {
			error("ERROR reading from socket");
			pthread_exit((void *)-1);
		}

		if (strcmp(buffer, "READY") == 0){
			READY = true;
		}
		if (strcmp(buffer, "HIT") == 0){
            gamewin->enemy_map[cds(my, mx)] = 7;
			werase(gamewin->enemyfield);
			print_field(gamewin->enemyfield, gamewin->win, 2, 93, " Enemy BattleField ");
			draw_map(gamewin->enemyfield, gamewin->enemy_map, false);
			wrefresh(gamewin->enemyfield);
		}
		if (strcmp(buffer, "MISS") == 0){
            gamewin->enemy_map[cds(my, mx)] = 8;
			werase(gamewin->enemyfield);
			print_field(gamewin->enemyfield, gamewin->win, 2, 93, " Enemy BattleField ");
			draw_map(gamewin->enemyfield, gamewin->enemy_map, false);
			wrefresh(gamewin->enemyfield);
		}
		if (buffer[0] == 'F'){
			int y = buffer[1]-'0';
			int x = buffer[2]-'0';
			if (gamewin->player_map[cds(y,x)] != 0 && gamewin->player_map[cds(y,x)] != 8){
                gamewin->player_map[cds(y,x)] = 7;
				n = write(CLIENT, "HIT", 3);
			} else {
                gamewin->player_map[cds(y,x)] = 8;
				n = write(CLIENT, "MISS", 4);
			}
			werase(gamewin->playerfield);
			print_field(gamewin->playerfield, gamewin->win, 2, 4, " Your Battlefield ");
			draw_map(gamewin->playerfield, gamewin->player_map, true);
			wrefresh(gamewin->playerfield);
			turn = true;
		}
		if (strcmp(buffer, "ACK") != 0){	
			n = write(CLIENT,"ACK",3);
		}
		if (strcmp(buffer, "DISC") == 0){
			CONNECTED = false;
			return;
		}
	}
}

void *init_host(void *gamewin){
	
	int oldt;	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldt);	

	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;

	SERVER = socket(AF_INET, SOCK_STREAM, 0);
	if (SERVER < 0) {
		error("ERROR opening socket");
		pthread_exit((void *)-1);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	if (bind(SERVER, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		error("ERROR on binding");
		pthread_exit((void *)-1);
	}

	listen(SERVER,5);
	clilen = sizeof(cli_addr);
	CLIENT = accept(SERVER, (struct sockaddr *) &cli_addr, &clilen);

	if (CLIENT < 0){
	       	error("ERROR on accept");
		pthread_exit((void *)-1);
	}
	
	CONNECTED = true;

	getparse_msg((game_win*)gamewin);

	close(SERVER);
	close(CLIENT);

	pthread_exit((void *)1);
}

void *init_client(void *gamewin){
	
	int oldt;	
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldt);	

	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	CLIENT = socket(AF_INET, SOCK_STREAM, 0);
	server = gethostbyname(ADDRESS);

	if (server == NULL) {
		error("ERROR, no such host");
		pthread_exit((void *)1);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(PORT);
	
	if (connect(CLIENT,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		error("ERROR connecting");
		pthread_exit((void *)1);
	}
	
	CONNECTED = true;

	getparse_msg((game_win*)gamewin);

	close(CLIENT);
	
	pthread_exit((void *)1);
} 

void multiplayer_host(){

    game_win *gamewin = create_gamewin();

	pthread_t tid;
	pthread_create(&tid, NULL, init_host, (void *)gamewin);

	wmove(gamewin->radiolog, 1, 3);
	wprintw(gamewin->radiolog, "Waiting for player... ");
	wrefresh(gamewin->radiolog);

	while(CONNECTED == false){
		wtimeout(gamewin->enemyfield, 5);
		int key = wgetch(gamewin->enemyfield);
		if (key == 27){ 
			wtimeout(gamewin->enemyfield, -1);
			close_gamewin(gamewin);
			close(SERVER);
			pthread_cancel(tid);
			return;
		}
		sleep(1);
	}
		
	wprintw(gamewin->radiolog, "Player connected. ");
	wrefresh(gamewin->radiolog);
	
	game_loop(gamewin, true);

	close_gamewin(gamewin);
}

void multiplayer_client(){

    game_win *gamewin = create_gamewin();

	pthread_t tid;
	pthread_create(&tid, NULL, init_client, (void *)gamewin);

	wmove(gamewin->radiolog, 1, 3);
	wprintw(gamewin->radiolog, "Connecting to host... ");
	wrefresh(gamewin->radiolog);

	while(CONNECTED == false){
		wtimeout(gamewin->enemyfield, 5);
		int key = wgetch(gamewin->enemyfield);
		if (key == 27){ 
			wtimeout(gamewin->enemyfield, -1);
			close_gamewin(gamewin);
			close(CLIENT);
			pthread_cancel(tid);
			return;
		}
		sleep(1);
	}
	
	wprintw(gamewin->radiolog, "Connected to host. ");
	wrefresh(gamewin->radiolog);
	
	turn = false;	
	game_loop(gamewin, true);

	close_gamewin(gamewin);
}

void print_main_menu(WINDOW* win, bool mp, int c){
	
	werase(win);
	box(win, 0, 0);	
	mvwprintw(win, 0, 3, mp ? " Multi Player " : " Main Menu ");
	mvwprintw(win, 9, 15, " Scroll:[UP, DOWN] Select:[ENTER] ");
    
	char *menu[3] = {"- Single Player", "- Multi Player", "- Exit"};
	char *mpmenu[3] = {"- Host Game", "- Join Game", "- Back"};
	for(int i = 0; i < 3; i++){
		if (i == c) wattron(win, A_REVERSE);
		mvwprintw(win, 2+(2*i), 6, mp ? mpmenu[i] : menu[i]);
		wattroff(win, A_REVERSE);
	}
    
	wrefresh(win);
}

void mph_menu(WINDOW *win){

	werase(win);
	box(win, 0, 0);	
	mvwprintw(win, 0, 3, " Multi Player ");
	mvwprintw(win, 4, 6, "- Port: ");
	wrefresh(win);
	char *p = calloc(6, sizeof(char));
	char *c = p;
	int key;
	do{
		key = wgetch(win);
		if (key == 263){
			int y, x;
			getyx(win, y, x);
			if (x > 14){
				mvwprintw(win, y, x-1, " ");
				wmove(win, y, x-1);
				*c = '\0';
				c--;
			}
		} else if (key == 10){
			curs_set(0);
			PORT= atoi(p);
            werase(win);
			multiplayer_host();
			break;
		} else if (key >= '0' && key <= '9') {
			wprintw(win, "%c", (char)key);
			*c = (char)key;
			c++;
		}
	} while (key != 27);
}

void mpc_menu(WINDOW *win){

	werase(win);
	box(win, 0, 0);	
	mvwprintw(win, 0, 3, " Multi Player ");
	mvwprintw(win, 3, 6, "- Address:");
	mvwprintw(win, 5, 6, "- Port:");
	wrefresh(win);
	char *p = calloc(16, sizeof(char));
	char *c = p;
	char *a = calloc(6, sizeof(char));
	char *d = a;
	int key;
	bool field = false;
	int y, x;
	getyx(win, y, x);
	wmove(win, y-2, 17);
	do{
		key = wgetch(win);
		if (key == 263){
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
		} else if (key == 10){
			curs_set(0);
			ADDRESS = a;
			PORT = atoi(p);
            werase(win);
			multiplayer_client();
			break;
		} else if (key >= '0' && key <= '9' || key == '.'){
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

void main_menu(){

	for(int i = 0; i < 29; i++) mvwprintw(master, 11, (COLS/2 - 29/2)+i, " ");
	wrefresh(master);

	WINDOW *mainwin = derwin(master, 10, 50, LINES/2, COLS/2-50/2);
	refresh();
	keypad(mainwin, true);

	int cursor = 0;
	int key;
	bool quit = false;
	bool multiplayer = false;
	do{
		do{
			if(cursor > 2) cursor = 2;
			if(cursor < 0) cursor = 0;

			print_main_menu(mainwin, multiplayer, cursor);

			key = wgetch(mainwin);
		
			if(key == KEY_UP) cursor--;
			if(key == KEY_DOWN) cursor++;
				
		} while (key != 10);
		switch(cursor){
			case 0:
				if(!multiplayer){
                    werase(mainwin);
					game_win *gamewin = create_gamewin();
					game_loop(gamewin, false);
					close_gamewin(gamewin);
				} else{
					curs_set(1);
					mph_menu(mainwin);
					curs_set(0);
				}
				break;
			case 1:
				if(!multiplayer){
					cursor = 0;
					multiplayer = true;
				} else {
					curs_set(1);
					mpc_menu(mainwin);
					curs_set(0);	
				}
				break;
			case 2:
				if(!multiplayer){
				       	quit = true;
				} else {
					cursor = 0;
					multiplayer = false;
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

	print_title_screen(master);
	getch();
	
	main_menu();	
	
	curs_set(1);
	endwin();

	return 0;

}

