Multi-Client Chat Server (CLI)
A real-time, command-line based multi-user chat application built using C and TCP socket programming, following a client–server architecture.
The server supports multiple concurrent clients using multi-threading and also includes an interactive Guess-the-Number game handled centrally by the server.

Features:
->Multiple clients can connect to a single server simultaneously

->Real-time message broadcasting to all connected clients

->Username-based identification for each client

->Server-side multi-threading (one thread per client)

->Graceful handling of client connections and disconnections

->Built-in interactive Guess-the-Number multiplayer game

->Command-line interface (CLI)

Tech Stack:
->Language: C

->Networking: TCP sockets (Winsock2)

->Concurrency: Multi-threading with synchronization

->Architecture: Client–Server model

-Platform: Windows

How It Works
->The server starts and listens on a fixed port for incoming client connections.

->Each connected client is handled in a separate thread, allowing concurrent communication.

->Messages sent by any client are broadcast to all other connected clients.

->The server maintains a list of connected users and handles join/leave events.

->The Guess-the-Number game is initiated by a command and managed entirely by the server.

->Clients receive real-time feedback (too high, too low, correct) until the game ends.

How to Run:
1️)Compile the Server
gcc server.c -o server -lws2_32

Compile the Client
gcc client.c -o client -lws2_32

3) Start the Server
./server

4) Start Clients (in separate terminals)
./client

Commands
Server Commands:
->/guessnum – Start the Guess-the-Number game

->/exit – Shut down the server

Client Commands:
->Type messages normally to chat

->/exit – Disconnect from the server

What We Learned:
->Implementing TCP socket communication from scratch

->Handling multiple clients using threads

->Synchronizing shared resources safely

->Debugging real-time networking issues

->Designing server-side logic for interactive features

->Understanding how real-time chat systems work internally

Future Improvements:
->Add a graphical user interface (GUI)

->Implement private messaging

->Add user authentication

->Encrypt messages for security

->Enable file transfer support

->Extend support beyond LAN (WAN/Internet
