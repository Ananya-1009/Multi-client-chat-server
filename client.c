#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>     // Contains all Windows socket functions: socket(), connect(), send(), recv()
#include <windows.h>
#include <process.h>      // For _beginthreadex() to create a receiving thread

#define PORT 8080
#define BUFFER_SIZE 1024

SOCKET sock;              // Global socket variable for client connection


// Thread for receiving messages from the server
unsigned __stdcall receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Continuously listen for incoming messages
    while ((bytes_read = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_read] = '\0';            
        printf("\r%s\n> ", buffer);           // Print received message above user input
        fflush(stdout);                       // Force clear output buffer
    }
    return 0;
}

int main() {
    WSADATA wsa;                              
    struct sockaddr_in server_addr;            // Stores server IP and port
    char buffer[BUFFER_SIZE];                 
    char username[50];                         // Stores client's username

    char final_msg[BUFFER_SIZE + 100];         

    // Startup WinSock (required before using any socket functions)
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

                                                
    sock = socket(AF_INET, SOCK_STREAM, 0); // Create client socket (IPv4, TCP)
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed.\n");
        WSACleanup();
        return 1;
    }

    // Get username from client
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';  // Remove newline character

    // Prepare server address
    server_addr.sin_family = AF_INET;           // IPv4
    server_addr.sin_port = htons(PORT);         // Convert port to network byte order
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP (change for LAN/WAN)
    

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Connection failed.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("\nConnected to server.\n");
    printf("Type messages normally.\n");
    printf("Commands:\n");
    printf("  /guessnum - start number guessing game\n");
    printf("  exit      - disconnect from server\n\n");

    // Send username first so server can register and announce it
    send(sock, username, (int)strlen(username), 0);

    // Start receiving messages in a separate thread
    _beginthreadex(NULL, 0, receive_messages, NULL, 0, NULL);

    // Main loop: send user input to the server
    while (1) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Strip newline

        if (strcmp(buffer, "exit") == 0)        // If user types exit → disconnect
            break;

        send(sock, buffer, (int)strlen(buffer), 0); // Send message to server
    }

    closesocket(sock);      // Close client socket
    WSACleanup();           // Cleanup Winsock
    return 0;
}
