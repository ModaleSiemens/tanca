# tanca
C++ multiplayer game.

## Ideas

Client App:

1. Asks whether the user wants to connect by direct server address and point or by it's name;

    By direct address:
    
    1. Prompts for server address and port;
    2. Tries to connect.

    By name:
    1. Prompts for server name;
    2. Sends request to Manager;
    3. Receives response: if a password is necessary, it asks it to the user

