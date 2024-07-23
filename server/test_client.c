#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>

#define ADDR "127.0.0.1"
#define PORT 8745


bool turn = false;
int self;
int CLIENT;

void error(const char *msg){
        perror(msg);
        exit(0);
}

void init_socket(){
	
	struct sockaddr_in serv_addr;
	struct hostent *server;

	// Open socket and get host
        CLIENT = socket(AF_INET, SOCK_STREAM, 0);
        if (CLIENT < 0) {
                error("ERROR opening socket");
        }

        server = gethostbyname(ADDR);
        if (server == NULL) {
                close(CLIENT);
                error("ERROR, no such host");           
        }
        
        // Clear and populate client address structs
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(PORT);
        
        // Connect to host
        if (connect(CLIENT,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
                close(CLIENT);
                error("ERROR connecting");
        }
	

}

void *getparse_msg(void *targ){

	int target;

	while(1){

		char *msgbuff = calloc(4, sizeof(char));

		int n = read(CLIENT, msgbuff, 4);
		if (n == 0) continue;
		if (n < 0){
			error("ERROR reading message");
		}
		
		if(sscanf(msgbuff, "%d", &target) != 0) {
			printf("Target: %d\n", target);
			continue;
		}

		printf("Server: %s\n", msgbuff);
		
		if(strcmp("TUR", msgbuff) == 0) turn = true;
				
		if(target == self){	
			if(msgbuff[0] == 'F'){ 
				write(CLIENT, "MIS", 4);
			}
		}
		free(msgbuff);
	}

}

int main(int argc, char **argv){

	init_socket();
	
	write(CLIENT, argv[1], 20);
	
	int num_players;

	read(CLIENT, &num_players, sizeof(int));

	struct msg_player{
		int id;
		char username[20];
	};

	struct msg_player *playerlist = calloc(num_players, sizeof(struct msg_player));

	read(CLIENT, playerlist, num_players * sizeof(struct msg_player));
	
	printf("Players:\n");
	for(int i = 0; i < num_players; i++) {	
		if(strcmp(argv[1], playerlist[i].username) == 0) self = playerlist[i].id;
		printf("\t%d-%s\n", playerlist[i].id, playerlist[i].username);
	}
	
	printf("self: %d\n", self);

	pthread_t tid;
	pthread_create(&tid, NULL, getparse_msg, (void *)argv[1]);

	write(CLIENT, "RDY", 4);

	while(1){
		
		sleep(1);
		if (turn == false) continue;
		
		char *buff = calloc(4, sizeof(char));
		
		printf("Target: ");
		fgets(buff, 5, stdin);	
		sscanf(buff, "%s", buff);
		
		write(CLIENT, buff, 4);

		printf("%s: ", argv[1]);
		fgets(buff, 5, stdin);	
		sscanf(buff, "%s", buff);

		write(CLIENT, buff, 4);

		turn = false;
	}

	return 0;
}
