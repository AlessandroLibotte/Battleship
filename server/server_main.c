#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

#define MAX_PLAYERS 3
#define PORT 8745

int SERVER, CLIENT;
struct sockaddr_in serv_addr;

typedef struct player{	
	int id;
	int des;
	char username[20];
	struct sockaddr_in *addr;
	bool ready;
} player;

void error(const char *msg){
	perror(msg);
	exit(0);
}

void init_socket(){

        // Open socket
        SERVER = socket(AF_INET, SOCK_STREAM, 0);
        if (SERVER < 0) {
                error("ERROR opening socket");
        }
        
        // Clear and populate socket address structs
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(PORT);

        // Bind socket address to server 
        if (bind(SERVER, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                close(SERVER);
                error("ERROR on binding");
        }
                
}

void waitingroom(player *playerlist){

	int num_players = 0;

	while(num_players < MAX_PLAYERS){
		
		printf("Waiting for connection... ");
		fflush(stdout);
		// Listen for client connection and accept connection
        	socklen_t clilen = sizeof(struct sockaddr_in);
        	playerlist[num_players].des = accept(SERVER, (struct sockaddr *) &(playerlist[num_players].addr), &clilen);
        	if (playerlist[num_players].des < 0){
        	        error("ERROR on accept");
        	}
		
		printf("Conenction established\n");
		playerlist[num_players].id = num_players;
		char buff[20];
		
		// Get player username
		if (read(playerlist[num_players].des, buff, 20) < 0){
			error("ERROR receiving message");
		}
		sscanf(buff, "%s", &(playerlist[num_players].username));

		printf("Player %s connected.\n\n", playerlist[num_players].username);
		num_players++;

	}

	// Print all connected players
	printf("Players:\n");
	for(int i = 0; i < MAX_PLAYERS; i++) printf("\t%d: %s\n", playerlist[i].id, playerlist[i].username);


}

void position_phase(player *playerlist){

	// Wait for end of position phase

	printf("Waiting for RDY messages...\n");

	int rdy_players = 0;
	
	// Set socket to non blocking
	for(int i = 0; i < MAX_PLAYERS; i++) fcntl(playerlist[i].des, F_SETFD, O_NONBLOCK);

	char *msg = calloc(4, sizeof(char));
	
	while(rdy_players < MAX_PLAYERS){
		// while not all the playersa are ready try to get ready message from a player if that player is not ready 
		for(int i = 0; i < MAX_PLAYERS; i++){
			if (playerlist[i].ready == true) continue;
			if (read(playerlist[i].des, msg, 4) > 0){
				if(strcmp(msg, "RDY") == 0) {
					rdy_players ++;
					printf("\t%d-%s: ready.\n", playerlist[i].id, playerlist[i].username);
					playerlist[i].ready = true;
					fflush(stdout);
				}
			}
		}
	}
	
	free(msg);

	// Set socket to blocking
	for(int i = 0; i < MAX_PLAYERS; i++) fcntl(playerlist[i].des, F_SETFD, 0);
	for(int i = 0; i < MAX_PLAYERS; i++) write(playerlist[i].des, "RDY", 4);

}

int main(){
	
	//Allocate space for player information
	player *playerlist = calloc(MAX_PLAYERS, sizeof(player));
	
	//start socket server
	printf("Starting socket... ");
	init_socket();
	printf("Socket started.\n\n");
	
        listen(SERVER,1);
	
	waitingroom(playerlist);
	
	// Send each player the number of players connected
	printf("Broadcasting playernum...");
	int n = MAX_PLAYERS;
	for(int i = 0; i < MAX_PLAYERS; i++) write(playerlist[i].des, &n, sizeof(int));
	printf("Done.\n\n");

	// Send playerlist
	struct msg_player{
		int id;
		char username[20];
	};

	struct msg_player *msg_playerlist = calloc(MAX_PLAYERS, sizeof(struct msg_player));

	for(int i = 0; i < MAX_PLAYERS; i++){
		msg_playerlist[i].id = playerlist[i].id;
		memcpy(msg_playerlist[i].username, playerlist[i].username, 20);
	}
	
	printf("Broadcasting playerlist...");
	for(int i = 0; i < MAX_PLAYERS; i++) 
		write(playerlist[i].des, msg_playerlist, MAX_PLAYERS * sizeof(struct msg_player));
	printf("Done.\n\n");

	// Broadcast the start message
	//printf("Sending start message...");
	//for(int i = 0; i < MAX_PLAYERS; i++) write(playerlist[i].des, "STR", 4);
	//printf("Done.\n\n");

	position_phase(playerlist);

	char *msg = calloc(4, sizeof(char));

	while(1){
		for(int i = 0; i < MAX_PLAYERS; i++){
			
			write(playerlist[i].des, "TUR", 4);
			int target;

			do{

				fflush(stdout);

				read(playerlist[i].des, msg, 4);
				
				//if the message is an int representing the target 
				if(sscanf(msg, "%d", &target) != 0){
					printf("%d-%s selected %d-%s as target\n", i, playerlist[i].username, target, playerlist[target].username);
				}else printf("%d-%s: %s\n", i, playerlist[i].username, msg);
				
				for(int j = 0; j < MAX_PLAYERS; j++) 
					//if (j != i)
					       	write(playerlist[j].des, msg, 4);
				
				
				if(msg[0] == 'F'){
					read(playerlist[target].des, msg, 4);
					for(int j = 0; j < MAX_PLAYERS; j++) 
						if(j != target) write(playerlist[j].des, msg, 4);
					printf("Traget %d-%s response: %s\n", target, playerlist[target].username, msg);
					break;
				}

			}while(1);

			/*
			int target;
			
			// read target 
			read(playerlist[i].des, &target, sizeof(int));
		       	printf("%d-%s selected target %d-%s", i, playerlist[i].username, target, playerlist[target].username);
			fflush(stdout);
			// read fire	
			read(playerlist[i].des, msg, 4);

			printf(": %s \n", msg);
			// fowrard attacker
			write(playerlist[target].des, &i, sizeof(int)); 
			// forward fire
			write(playerlist[target].des, msg, 4);

			//read response 
			read(playerlist[target].des, msg, 4);
			
			printf("%d-%s response: %s\n", target, playerlist[target].username, msg);

			//forward response 
			write(playerlist[i].des, msg, 4);
			
			fflush(stdout);
			*/
		}
	}

	free(msg);

	return 0;
}
