# tanca
C++ singleplayer and multiplayer 2D game.

## Ideas

Client App:

1. Asks whether the user wants to connect by direct server address and point or by it's name;

    By direct address:
    
    1. Prompts for server address and port;
    2. Tries to connect.

    By name:
    1. Prompts for server name and password
    2. Server Manager might respond with
        - server_not_found 
        - server_full
        - wrong_password
        - connection_refused + string

