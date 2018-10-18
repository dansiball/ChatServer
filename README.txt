Here is the documentation to our glorious chat server! Enjoy!

Both the Server and Client were compiled on our beloved linux server Skel.
The hardcoded ports for our port knocking are: 9985, 9986 and 9987

 ----  Server  ----
To build and run:
g++ server.cpp -o server
./server

 ----  Client  ----
 To build and run:
 g++ client.cpp -o client
./client <host> <port knocking 1> <port knocking 2> <main port>

Commands:
HELP                - Provides all possible commands
ID                  - Provides ID
CONNECT <USERNAME>  - Connect as a user
CHANGE ID           - Changes ID
LEAVE               - Exit
WHO                 - List of connected users

    -- Possible commands after connecting as user --
    MSG <USERNAME>  - Send private message to user
    MSG ALL         - Send message to all connected users

 ---- Stolen ID's  ----
 We built this port scanner on the port scanner from our previous assignment.
 We were unable to hold any records of our own ID being stolen, although we noticed it a few times while our server was running.

Port: 55002 55001 55000 -OPEN
We've picked COBOL as the language of choice.
DÖMBAETHJ
1538345203

Port: 75521 75522 75523 - OPEN
Server ID: Battle, n.:  A method of untying with the teeth a political knot that will not yield to the tongue. -- Ambrose Bierce
AAB
21:45:51 09/30/18

Port: 71091 71092 71093 - OPEN
Before you ask more questions, think about whether you really want to know the answers. -- Gene Wolfe, "The Claw of the Conciliator"
1538343516
Group 49

Port: 51234 51236 51235  - OPEN
love, n.: When you don't want someone too close--because you're very sensitive to pleasure.
Group21 
30-09-2018 22-09-07

 ----  Known Errors  ----
About once every 20/30 times we try to connect with the server using our client (with the correct port knocking), we get an unknown error and we can't connect.
This has something to do with the fact that the client tries to connect using the third port first and gets refused.
But we fixed this using usleep(500) in client.cpp, so that we sleep for a very short time between port knocks.