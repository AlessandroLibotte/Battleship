# Battleship by Alessandro Libotte
A TTy retro implementation of the classic game with some modern twists

> This project started as a personal challenge to learn how to create terminal GUI applications using the curses.h library and then evolved into the final project for my University's Operating Systems course.

## Core data structures used
Throughout the application a number of data structures are used, either library defined or custom made. Organizing the data handled by the application this way was a decision made mainly for ease of development, general code tidiness and especially 
to reduce the number of different arguments passed to functions.
### The WINDOW data structure
The application, being mainly GUI oriented from its conception, heavily relies on the ```WINDOW``` data structure defined by the curses.h library. This data structure, as the name implies, is used to keep track of and modify windows made up of portions of the terminal window in which the application is run. Windows can be either created or derived from an existing window.
The application is structured on top of a master window from which three major window blocks are derived. The first major window block that the user encounters is the ```main_menu``` window block, this window block allows the user to navigate through the features of the application allowing them to: 

- Start a single player match against a coded opponent
- Host a classic 1v1 match on the machine running the application
- Join a classic 1v1 match being hosted on a different machine
- Access a user created multiuser match server list

When a user either starts a single player match, hosts or joins a match the second major window block is initialized and displayed. This window block is handled by the application through the use of the ```game_win``` struct, this struct stores pointers to all the windows that make up the main game interface and two pointers to the arrays representing the player map and enemy map, where the ships positions and the hit and miss markers are stored. This window block is composed of the following windows:

- win: the main window derived from the master window and from which all the following windows are derived.
- radiolog: the window dedicated to screen logs of the action taken by every player during a match
- playerfield: the window that displays the player's field showing the ships and the hit and miss markers
- enemyfield: the window that displays the enemy's field with only the hit and miss markers 
- playerfleet: the window dedicated to showing the remaining player's ship
- enemyfleet: the window dedicated to showing the remaining enemy's ship

If the user instead wishes to connect to a multiuser match they will encounter the third major window block, the server list window block. Before accessing this window block the application checks for the presence of the ```multiuser.conf``` file in the current directory, this file is used to store the user's name, the total number of servers and the list of saved servers. If at the moment of accessing the serverlist menu the config file is not found, the user is prompted to provide a username that is mandatory for accessing a multiuser game. After that the user is presented with a list of the servers stored on the file, if there are any, and is able to perform the followin actions on them:

- Connect to the currently selected server
- Edit the currently selected server
- Delete the currently selected server
- Add a new server
### Inter Process Communication data structures
To ensure a good communication between the process's threads two data structures are used:

- ```t_targ```: used as the argument for the application's threads
- ```msg```: used with the system's message queues to enable communication between the threads

The ```t_targ``` structure includes the following elements:

- the file descriptor relative to the message queue opened by the main thread
- a pointer to the game_win data structure being displayed on the screen
- a pointer to a string containing the ip address the users wished to connect to
- the port number where the match is being hosted

The ```msg``` structure is defined as is described in the msgrcv POSIX Programmer's Manual.
### Data structure used for storing 
To implement persistance when accessing the user created multiuser server list, the corresponding data is stored on the file system and it is handled by the application trough the use of the server_t structure. This structure stores the name, ip address and port of a server. Every time the user accesses the multiuser server list, if present, the ```multiuser.conf``` file is opened and after reading the number of servers the space for the correct number of structures is allocated in a list, then the structures are populated with the servers data read from file in a looping fashion. 
If the user decides to edit or delete a server then the list containing the servers is duplicated except for the deleted or edited server, that is either skipped or substituted, and the file is updated.

## Input 
Troughout the application, input is handled in both a blocking and non-blocking fashion depending on the circumstances. 

The input is captured using the ```wgetch()``` function, a function implemented by the curses.h library that allows input capture for a specified curses window. By default the call to ```wgetch()``` is blocking but this behaviour can be changed with a call to the ```wtimeout()``` function that allows to set a specific input timeout in milliseconds for a specific curses window, if the specified timeout is negative the behaviour will be blocking, else if no input is provided before the duration of the specified timeout ```wgetch()``` will return an error code.

Whether the input is handled in a blocking or non-blocking fashion depends on the current state of the application. If the application is in a static state (e.g. a menu screen), where a change to the current state of the application can be caused only from some input, then it is handled in a blocking fashion to preserve the machine's resource. Else if the application is in a dynamic state (e.g. during a match), where a change in the application state could be caused by external events, the input is handled in a non-blocking fashion to allow for the reception of these external events.

Since the application mostly requires multiple inputs in a succession to perform an action (e.g. select a menu option with the arrow keys then access it with the enter key), the input is always captured insiede loops of similar fashion.

Basic menu input loop example:
```
int cursor = 0;
int key = 0;

do{

  //check cursor boundaries
  if (cursor > 2) cursor = 2;
  if (cursor < 0) cursor = 0;

  print_menu(win, cursor);

  key = wgetch(win);

  if (key == KEY_UP) cursor--;
  if (key == KEY_DOWN) cursor++;
  if (key == 10){ // if the user pressed ENTER
    switch(cursor){
      // do something
    }
  }

}while(key != 27) // while the pressed key is not ESC

```

In some cases the user is required to provide a text input alongside arrow keys, enter and esc presses (e.g. when entering a username, ip address or  port number). 

To manage this, two pointers are initialized outside the input loop for each input field. One pointer remains unchanged and is used to store the input, while the other serves as a cursor within the input field. By leveraging pointer arithmetic, the length of the inputted text can be dynamically calculated (string length = cursor pointer position - string pointer position). This allows the cursor to maintain its position at the end of the text, regardless of the varying lengths of input strings, when transitioning between input fields. Within the input loop, the integer value of the pressed key is examined. If the key falls within the respective ranges of alphanumeric characters required for the specific input field, such as ('a'-'z', 'A'-'Z', or '0'-'9') and the cursor doesn't exceed the maximum string length, the pressed key is printed to screen, the cursor pointer for that field is updated with the corresponding character value and then advanced to the next position. If the pressed key is backspace and the string lenght is grather than zero the cursor pointer for the current field is set to '\0' and then decremented.

(For the most comprehensive implementation of this technique, refer to the 'add_server()' function)

## Game loop
The ```game_loop()``` function is responsible for handling any kind of match such as single player, online or multiuser.

### Single player matches
When the user starts a single player match, after the initialization of the ```game_win``` object, the ```game_loop()``` function is called with the ```mp``` boolean argument set to ```false``` signaling that the match is single player. From this point on if the ESC key is presses at any time the user will be able to quit the game and return to the main menu.

First of all the user will enter the positioning phase handled by the ```position_fleet()``` function, this function enables the user to move around, rotate and place 5 ships inside of his field's borders. On completion of the user's positioning phase the enemy's ships are randomply rotated and placed inside the enemy field and the actual match can start.

The game loop is basically comprised of two sections: one for the users's turn and one for the enemy's turn. The user's turn is an input loop that allows the user to move a cursor over the enemy field and fire by checking the enemy map at the current cursor position and setting it to the hit or miss marker accordingly, while the enemy's turn randomly chooses a point on the player field to fire upon then checks the player map at the cursor position and sets it to the hit or miss marker accordingly.

When either one of the turns has ended the turn is respectively passed to the user or to the enemy.

### Online matches
When the user starts an online match, after the initialization of the ```game_win``` object, the ```game_loop()``` function is called with the ```mp``` boolean argument set to ```true``` signaling that the match is online. From this point on if the ESC key is presses at any time the user will be able to quit the game and return to the main menu.

First of all the user will enter the positioning phase handled by the ```position_fleet()``` function, this function enables the user to move around, rotate and place 5 ships inside of his field's borders. On completion of the user's positioning phase the message "RDY" is writtrn to the socket signaling to the enemy's application that the user's application is ready to commence the match, then it enters an infinite loop waiting for the reception of the enemy's "RDY" message on the inter process message queue.

When both users are ready the actual match can start and the host is always the first to play. One at a time both users will be able to move the cursor on the enemy field and fire upon the selected position. When a user decides to fire, a string containing the user's firing coordinates is dynamically created and written to both the socket and the inter process message queue. The actual consequenses of the firing event are handled in the ```getparse_msg()``` function (refer to the **Multiplayer** paragraph). After fireing, the application will enter an infinite loop waiting for either the user to press the ESC or a message on the inter process message queue signaling that the turn has been passed back or the disconnection of the enemy.

### Multiuser matches !!!NOT IMPLEMENTED YET!!!
The concept should remain essentially the same as the online match, with the additional feature of being able to select which enemy to fire upon via a selector field that will replace the "Enemy Field" text with "[Username]'s Field".

## Multiplayer
Most of the multiplayer capabilities of the application are handled by four functions: ```multiplayer()```, ```init_client()```, ```init_host()``` and ```getparse_msg()```. Since hosting and connecting to a match require some quite similar routines, both actions share most of the code, with the major difference being in the ```init_client()``` and ```init_host()``` functions.

When either joining or hosting an online match, the ```multiplayer()``` function is called with the boolean argument ```mode``` signaling wich of the two actions the user wishes to take. The  ```multiplayer()``` function is responsible for the installation of the inter process message queue and the creation of a thread that will handle the socket communication. To do so, fisrts the ```game_win``` object is created, the inter process message queue is exclusively installed and depending on the ```mode``` argument value a thread is created with the ```init_client()``` or ```init_host()``` function as the target. After that the main thread enters an infinite loop waiting for the press of the ESC to abort or the reception of either the "CON" or "ERR" message. On reception of the "CON" message the ```game_loop()``` function is called and the main thread procedes as described in the **Online matches** subsection of the **Game loop** paragraph. If instead the ESC key is pressed or the "ERR" message is recived the main thread will terminate the other thread, close the socket connection and return the user to the main menu.

In the meantime, while the main thread is waiting for the "CON" message on the inter process message queue, the other thread will setup the socket connection, either server or client depending on whitch function is set as target for the thread, and will then wait for a connection or attempt to connect to the host provided by the user. If at any point of this procedure one of the system calls fails, the "ERR" message will be sent on the inter process message queue and the thread will terminate after closing the socket connection if it has been opened. Otherwise if a connection is succesfully established, while the main thread enteres the ```game_loop()``` function, the ```getparse_msg()``` function is called by the thread.

The ```getparse_msg()``` function, as the name implies, is responsible for the reception and parsing of the messages recived on the socket. To achieve this, the thread enters a loop that will be repeated untill one of the user disconnects. 

Inside this loop the thread will 


