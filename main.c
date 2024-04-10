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
WINDOW *gamewin;
WINDOW *radiolog;
WINDOW *playerfield; 
WINDOW *enemyfield;
WINDOW *playerfleet;
WINDOW *enemyfleet;
WINDOW *mainwin;

bool CONNECTED = false;
bool READY = false;
bool THREXIT = false;
int CLIENT, SERVER;

char *ADDRESS;
int PORT;

bool turn = true;
char *player_map;
char *enemy_map;
int mx, my;

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
		mvwprintw(master, 7+i, (COLS/2)-(title_len/2), title[i]);
	}
	box(master, 0, 0);
	wattron(master, A_BLINK);
	int msg_len = 29;
	mvwprintw(master, 11, COLS/2-msg_len/2, "Press any button to continue");	
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
	enemyfield = derwin(gamewin, 21, 41, 2, 93);
	playerfleet = derwin(gamewin, 21, 21, 2, 48);
	enemyfleet = derwin(gamewin, 21, 21, 2, 69);
	radiolog = derwin(gamewin, 7, 130, 23, 4);
	refresh();

	print_field(playerfield, 2, 4, " Your Battlefield ");
	print_fleet(playerfleet, " Your Fleet ");
	
	print_field(enemyfield, 2, 93, " Enemy BattleField ");
	print_fleet(enemyfleet, " Enemy Fleet ");

	box(radiolog, 0, 0);
	mvwprintw(radiolog, 0, 3, " Radio Log ");
	
	wrefresh(playerfield);
	wrefresh(enemyfield);
	wrefresh(playerfleet);
	wrefresh(enemyfleet);
	wrefresh(radiolog);
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
	char *map = calloc(100, sizeof(char));
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
		draw_map(playerfield, map, true);		
		
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
			bool placed = true;
			if (rot){
				for(int i = 0; i < ship[rem_ships]+1; i++) if (map[cds(y,x+i)] != 0) placed = false;
			        if (placed){	
					map[cds(y,x)] = 1;
					for (int i = 1; i < ship[rem_ships]; i++) map[cds(y,x+i)] = 2;
					map[cds(y,x+ship[rem_ships])] = 3;
				}
			}
			else{
				for(int i = 0; i < ship[rem_ships]+1; i++) if (map[cds(y+i,x)] != 0) placed = false;
			        if (placed){
					map[cds(y,x)] = 4;
					for (int i = 1; i < ship[rem_ships]; i++) map[cds(y+i,x)] = 5;
					map[cds(y+ship[rem_ships], x)] = 6;
				}
			}
			if (placed) rem_ships--;
		}
		if (key == 27){
			*map = 10;
			break;
		}
	} while(rem_ships >= 0);

	keypad(playerfield, false);
	werase(playerfield);
	print_field(playerfield, 2, 4, " Your Battlefield ");
	draw_map(playerfield, map, true);
	wrefresh(playerfield);

	return map;

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

void game_loop(bool mode){

	player_map = position_fleet();

	if(*player_map != 10){

		if (mode) {
			write(CLIENT,"READY", 6);
			while(!READY) sleep(1);
			enemy_map = calloc(100, sizeof(char));
			wtimeout(enemyfield, 5);
		}
		else enemy_map = setup_enemy();

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
				print_field(enemyfield, 2, 93, " Enemy BattleField ");
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
					if (mode){
						char msg[4] = {'F', '0'+y, '0'+x, '\0'};
						mx = x;
						my = y;
						int n = write(CLIENT,(char*)msg, 4);
					} else {
						if(enemy_map[cds(y,x)] != 0 && enemy_map[cds(y,x)] != 8){
							enemy_map[cds(y,x)] = 7;	
						} else {
							enemy_map[cds(y,x)] = 8;
						}
					}
					turn = false;
				}
				wrefresh(enemyfield);
			} else {
				if (mode){
					while(!turn && CONNECTED && key != 27) {
						key = wgetch(enemyfield);
						sleep(1);
					}
				} else {
					int ex, ey;
					
					ex = rand() % 10;
					ey = rand() % 10;
						
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
			}
	
			if(mode && !CONNECTED) break;
		}while(key != 27);
		
		keypad(enemyfield, false);
	
		free(enemy_map);
	}
	if(mode){
		write(CLIENT, "DISC", 4);
		wtimeout(enemyfield, -1);
		CONNECTED = false;
	}
	free(player_map);

}

void create_gamewin(){
	gamewin = derwin(master, LINES-(LINES/3)-1, 138, LINES/3, (COLS/2)-(138/2)); 
	refresh();
	box(gamewin, 0, 0);
	mvwprintw(gamewin, 30, 138-72, " Move:[UP,DOWN,LEFT,RIGHT] Place Ship/Fire:[ENTER] Rotate Ship:[SPACE] ");
	setup_fields();
	wrefresh(gamewin);
}

void close_gamewin(){
	werase(playerfield);
	werase(playerfleet);
	werase(enemyfield);
	werase(enemyfleet);
	werase(radiolog);
	werase(gamewin);
	wrefresh(playerfield);
	wrefresh(playerfleet);
	wrefresh(enemyfield);
	wrefresh(enemyfleet);
	wrefresh(radiolog);
	wrefresh(gamewin);
	refresh();
}

void error(const char *msg){
	werase(mainwin);
	WINDOW *errwin = derwin(master, 10, 40, (LINES/2)-5, (COLS/2)-20);
	refresh();
	box(errwin, 0, 0);
	mvwprintw(errwin, 0, 3, " !ERROR! ");
	mvwprintw(errwin, 4, 6, "%s", msg);
	wrefresh(mainwin);
	wrefresh(errwin);
	wgetch(errwin);
	werase(errwin);
	wrefresh(errwin);
}

void getparse_msg(){

	int n;
	char *buffer; 
	
	while(CONNECTED){
		
		if (n < 0) {
			error("ERROR reading from socket");
			pthread_exit((void *)-1);
		}
	
		buffer = calloc(256, sizeof(char));
	
		n = read(CLIENT,buffer,255);
		
		if (CONNECTED){	
			wprintw(radiolog, "%s ", buffer);
			wrefresh(radiolog);
		}
		if (strcmp(buffer, "READY") == 0){
			READY = true;
		}
		if (strcmp(buffer, "HIT") == 0){
			enemy_map[cds(my, mx)] = 7;
			werase(enemyfield);
			print_field(enemyfield, 2, 93, " Enemy BattleField ");
			draw_map(enemyfield, enemy_map, false);
			wrefresh(enemyfield);

		}
		if (strcmp(buffer, "MISS") == 0){
			enemy_map[cds(my, mx)] = 8;
			werase(enemyfield);
			print_field(enemyfield, 2, 93, " Enemy BattleField ");
			draw_map(enemyfield, enemy_map, false);
			wrefresh(enemyfield);

		}
		if (buffer[0] == 'F'){
			int y = buffer[1]-'0';
			int x = buffer[2]-'0';
			if (player_map[cds(y,x)] != 0 && player_map[cds(y,x)] != 8){
				player_map[cds(y,x)] = 7;
				n = write(CLIENT, "HIT", 3);
			} else {
				player_map[cds(y,x)] = 8;
				n = write(CLIENT, "MISS", 4);
			}
			werase(playerfield);
			print_field(playerfield, 2, 4, " Your Battlefield ");
			draw_map(playerfield, player_map, true);
			wrefresh(playerfield);
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

void *init_host(void *port){

	int client;
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
	client = accept(SERVER, (struct sockaddr *) &cli_addr, &clilen);

	if (client < 0){
	       	error("ERROR on accept");
		pthread_exit((void *)-1);
	}
	
	CLIENT = client;
	CONNECTED = true;

	getparse_msg();

	close(SERVER);
	close(CLIENT);

	THREXIT = true;

	pthread_exit((void *)1);
}

void *init_client(){

	int client;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	
	client = socket(AF_INET, SOCK_STREAM, 0);
	server = gethostbyname(ADDRESS);

	if (server == NULL) {
		error("ERROR, no such host\n");
		pthread_exit((void *)1);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(PORT);
	
	if (connect(client,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		error("ERROR connecting");
		pthread_exit((void *)1);
	}
	
	CLIENT = client;
	CONNECTED = true;

	getparse_msg();

	close(CLIENT);
	
	THREXIT = true;

	pthread_exit((void *)1);
} 

void multiplayer_host(){
	
	pthread_t tid;
	pthread_create(&tid, NULL, init_host, NULL);

	werase(mainwin);
	create_gamewin();
	wmove(radiolog, 1, 3);
	wprintw(radiolog, "Waiting for player... ");
	wrefresh(radiolog);

	while(CONNECTED == false){
		wtimeout(enemyfield, 5);
		int key = wgetch(enemyfield);
		if (key == 27){ 
			wtimeout(enemyfield, -1);
			close_gamewin();
			return;
		}
		sleep(1);
	}
		
	wprintw(radiolog, "Player connected. ");
	wrefresh(radiolog);
	
	game_loop(true);

	close_gamewin();	
}

void multiplayer_client(){
	
	pthread_t tid;
	pthread_create(&tid, NULL, init_client, NULL);

	werase(mainwin);
	create_gamewin();
	wmove(radiolog, 1, 3);
	wprintw(radiolog, "Connecting to host... ");
	wrefresh(radiolog);

	while(CONNECTED == false){
		wtimeout(enemyfield, 5);
		int key = wgetch(enemyfield);
		if (key == 27){ 
			wtimeout(enemyfield, -1);
			close_gamewin();
			return;
		}
		sleep(1);
	}
	
	wprintw(radiolog, "Connected to host. ");
	wrefresh(radiolog);
	
	turn = false;	
	game_loop(true);

	close_gamewin();

}

void main_menu(){
	
	int msg_len = 29;
	for(int i = 0; i < 29; i++) mvwprintw(master, 11, (COLS/2 - msg_len/2)+i, " ");
	wrefresh(master);
	
	int win_len = 50;
	mainwin = derwin(master, 10, win_len, LINES/2, COLS/2-win_len/2);
	refresh();
	keypad(mainwin, true);

	int cursor = 0;
	int key;
	bool quit = false;
	bool multiplayer = false;
	do{
		do{
			box(mainwin, 0, 0);	
			mvwprintw(mainwin, 0, 3, multiplayer ? " Multi Player " : " Main Menu ");
			mvwprintw(mainwin, 9, 15, " Scroll:[UP, DOWN] Select:[ENTER] ");
	
			if(cursor > 2) cursor = 2;
			if(cursor < 0) cursor = 0;
	
			char *menu[3] = {"- Single Player", "- Multi Player", "- Exit"};
		        char *mpmenu[3] = {"- Host Game", "- Join Game", "- Back"};
			for(int i = 0; i < 3; i++){
				if (i == cursor) wattron(mainwin, A_REVERSE);
				mvwprintw(mainwin, 2+(2*i), 6, multiplayer ? mpmenu[i] : menu[i]);
				wattroff(mainwin, A_REVERSE);
			}	
			
			wrefresh(mainwin);
	
			key = wgetch(mainwin);
		
			if(key == KEY_UP) cursor--;
			if(key == KEY_DOWN) cursor++;
				
			werase(mainwin);
		} while (key != 10);
		switch(cursor){
			case 0:
				if(!multiplayer){
					create_gamewin();					
					game_loop(false);
					close_gamewin();
				} else{
					box(mainwin, 0, 0);	
					mvwprintw(mainwin, 0, 3, " Multi Player ");
					mvwprintw(mainwin, 4, 6, "- Port: ");
					wrefresh(mainwin);
					curs_set(1);
					char *p = calloc(6, sizeof(char));
					char *c = p;
					int key;
					do{
						key = wgetch(mainwin);
						if (key == 263){
							int y, x;
							getyx(mainwin, y, x);
							if (x > 14){
								mvwprintw(mainwin, y, x-1, " ");
								wmove(mainwin, y, x-1);
								*c = '\0';
								c--;
							}
						} else if (key == 10){
							curs_set(0);
							PORT= atoi(p);	
							multiplayer_host();
							break;
						} else if (key >= '0' && key <= '9') {
							wprintw(mainwin, "%c", (char)key);
							*c = (char)key;
							c++;
						}
					} while (key != 27);
					curs_set(0);
					werase(mainwin);
				}
				break;
			case 1:
				if(!multiplayer){
					cursor = 0;
					multiplayer = true;
				} else {
					box(mainwin, 0, 0);	
					mvwprintw(mainwin, 0, 3, " Multi Player ");
					mvwprintw(mainwin, 3, 6, "- Address:");
					mvwprintw(mainwin, 5, 6, "- Port:");
					wrefresh(mainwin);
					curs_set(1);
					char *p = calloc(16, sizeof(char));
					char *c = p;
					char *a = calloc(6, sizeof(char));
					char *d = a;
					int key;
					bool field = false;
					int y, x;
					getyx(mainwin, y, x);
					wmove(mainwin, y-2, 17);
					do{

						key = wgetch(mainwin);
						if (key == 263){
							getyx(mainwin, y, x);
							if (x > (field ? 14 : 17)){
								mvwprintw(mainwin, y, x-1, " ");
								wmove(mainwin, y, x-1);
								if (field){
									*c = '\0';
									c--;
								} else {
									*d = '\0';
									d--;	
								}
							}
						} else if(key == KEY_UP && field){
							getyx(mainwin, y, x);
							field = false;
							wmove(mainwin, y-2, 17+(d-a));
						} else if(key == KEY_DOWN && !field){
							getyx(mainwin, y, x);
							field = true;
							wmove(mainwin, y+2, 14+(c-p));
						} else if (key == 10){
							curs_set(0);
							ADDRESS = a;
							PORT = atoi(p);
							multiplayer_client(a, p);
							break;
						} else if (key >= '0' && key <= '9' || key == '.'){
							wprintw(mainwin, "%c", (char)key);
							if(field){
								*c = (char)key;
								c++;
							} else {
								*d = (char)key;
								d++;
							}
						}
					} while (key != 27);
					curs_set(0);	
					werase(mainwin);
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
	print_title_screen();
	getch();
	
	main_menu();	
	
	curs_set(1);
	endwin();

	return 0;

}

