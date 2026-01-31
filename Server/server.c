#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h> //it conatins all the networking functions like socket bind listen accept send recv
#include <process.h> //this is for threading to connect server with each client individually (multithreading)
#include <ws2tcpip.h> //winsock extensions for inet_pton which converts string(ip address) to binary
#include <time.h>  // used for random function

#pragma comment(lib, "ws2_32.lib") // links the code to winsock library

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

SOCKET clients[MAX_CLIENTS]; // will create an array which store sockerts of clients
char usernames[MAX_CLIENTS][50]; //will create array to store usernames of clients with same indexing as their sockets
CRITICAL_SECTION clients_mutex; //it is a windows synchronization object to allow only one thread to access at a time


int game_active = 0;
int secret_number = 0;

// This function basically broadcasts a message on screen from one client or from server itself to the other clients connected to the server
void broadcast(const char *message, SOCKET sender_sock) {
    EnterCriticalSection(&clients_mutex); //allows only one thread to work on
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0 && clients[i] != sender_sock) { //ensures message is sent to other clients not the one who sent it
            send(clients[i], message, (int)strlen(message), 0);
        }
    }
    LeaveCriticalSection(&clients_mutex);
}

// ig it's obvious
void start_guess_game() {
    if (game_active) {
        broadcast("A game is already running.\n", 0);
        return;
    }
    secret_number = rand() % 100 + 1;
    game_active = 1;
    broadcast("Guess the Number game started! Guess from 1 to 100.\n", 0);
    printf("[GAME] Secret number is %d (hidden)\n", secret_number);
}

//this is the imp function that will handle all the queries related to a client
void handle_client(void *arg) {  // void *arg converts any data type into a pointer to use
    SOCKET sock = *(SOCKET *)arg; // Convert void* argument back to actual SOCKET by casting and dereferencing
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int idx = -1;

    // Find the client index
    EnterCriticalSection(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == sock) {
            idx = i;                
            break;
        }
    }
    LeaveCriticalSection(&clients_mutex);

    // Receive username
    bytes_read = recv(sock, usernames[idx], sizeof(usernames[idx]) - 1, 0);
    if (bytes_read <= 0) {
        closesocket(sock);
        _endthread();        // if no username is given then the server closes the thread with the client
        return;
    }
    usernames[idx][bytes_read] = '\0'; //adds end of line at the end of username

    // Announce join
    char join_msg[BUFFER_SIZE];
    snprintf(join_msg, sizeof(join_msg), "%s joined the chat.\n", usernames[idx]);  //creates a formatted message
    printf("%s", join_msg);
    broadcast(join_msg, sock); // sends to other clients

    // Main message loop
    while ((bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';

        // Game command
        if (strncmp(buffer, "/guessnum", 9) == 0) { //compares if the message written by client is same as /guessnum to start the game
            start_guess_game();
            continue;
        }

        // Guess handling
        if (game_active) {
            int guess = atoi(buffer); //converts string to integer ASCII TO INT
            if (guess >= 1 && guess <= 100) {
                if (guess == secret_number) {
                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg),
                             "%s guessed %d correctly! GAME OVER.\n",
                             usernames[idx], secret_number);
                    broadcast(msg, 0);
                    printf("%s", msg);
                    game_active = 0;
                } else if (guess < secret_number) {
                    char hint[BUFFER_SIZE];
                    snprintf(hint, sizeof(hint), "%s guessed %d - too low.\n",
                             usernames[idx], guess);
                    broadcast(hint, 0);
                } else {
                    char hint[BUFFER_SIZE];
                    snprintf(hint, sizeof(hint), "%s guessed %d - too high.\n",
                             usernames[idx], guess);
                    broadcast(hint, 0);
                }
                continue;
            }
        }

        // NORMAL CHAT MESSAGE — prefix username
        char formatted_msg[BUFFER_SIZE + 100];
        snprintf(formatted_msg, sizeof(formatted_msg),  //creates a formatted message where username comes before the message written by user
                 "%s: %s\n", usernames[idx], buffer);

        broadcast(formatted_msg, sock);
    }

    // Announce leaving
    char leave_msg[BUFFER_SIZE];
    snprintf(leave_msg, sizeof(leave_msg), "%s left the chat.\n", usernames[idx]);
    printf("%s", leave_msg);
    broadcast(leave_msg, sock);

    // Remove client
    EnterCriticalSection(&clients_mutex);  //removes clients one at a time
    clients[idx] = 0;
    usernames[idx][0] = '\0';
    LeaveCriticalSection(&clients_mutex);

    closesocket(sock);
    _endthread();
}

void server_input_thread(void *arg) {  //this function basically is for server to broadcast a message or to control the server
    char msg[BUFFER_SIZE];
    while (1) {
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0';

        if (strcmp(msg, "/exit") == 0) {
            printf("Shutting down server...\n");
            broadcast("SERVER shutting down.\n", 0);
            exit(0);
        } else if (strcmp(msg, "/guessnum") == 0) {
            start_guess_game();
            continue;
        }

        char final_msg[BUFFER_SIZE];
        snprintf(final_msg, sizeof(final_msg),
                 "SERVER: %s\n", msg);
        broadcast(final_msg, 0);
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_sock;
    struct sockaddr_in server_addr, client_addr;
    int addr_len = sizeof(client_addr);

    srand((unsigned int)time(NULL)); //Seeds the random number generator for the Guess-the-Number game.

    WSAStartup(MAKEWORD(2, 2), &wsa); // to initialize networking winsock api
    InitializeCriticalSection(&clients_mutex);  //creates a lock to protect data getting accessed by multiple threads
    memset(clients, 0, sizeof(clients));  //sets every client socket to 0 to show empty slots

    server_fd = socket(AF_INET, SOCK_STREAM, 0);  //creates tcp socket for server with AF_INET-> IPV4 AND SOCK_STREAM-> TCP
    if (server_fd == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;         //fill server address structure
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT); //htons convert port to bytes 

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        return 1;
    }

    if (listen(server_fd, 10) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        return 1;
    }

    printf("Server running on port %d...\n", PORT);
    printf("Commands:\n");
    printf("  /exit\n");
    printf("  /guessnum\n\n");

    _beginthread(server_input_thread, 0, NULL); //creates seperate thread for server to write commands and send messages

    while (1) {
        new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len); //accepts new client
        if (new_sock == INVALID_SOCKET) continue;

        EnterCriticalSection(&clients_mutex);
        int added = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == 0) {
                clients[i] = new_sock;          //looks for empty slot to store new client's socket
                added = 1;
                break;
            }
        }
        LeaveCriticalSection(&clients_mutex);

        if (added) {
            _beginthread(handle_client, 0, &new_sock);  // Create a new thread that runs handle_client(), using default stack size, and pass the client socket as argument
        } else {
            char *msg = "Server full.\n";
            send(new_sock, msg, (int)strlen(msg), 0); // if server is full the it sends a message and ends connection
            closesocket(new_sock);
        }
    }

    return 0;
}
