#pragma once

#include <string>

struct ServerData
{
    std::string name;
    
    bool password_required;

    std::size_t players_count;
    std::size_t max_player_count;
};

enum class Messages
{
    // mdsm::nets requested messages
    ping_request,
    ping_response,

    // Messages sent from client
    client_server_list_request,
    client_connection_request,
    client_probe,

    // Messages sent from server
    server_connection_refused,
    server_wrong_password,
    server_probe,
    server_full,

    // Messages sent from server manager
    server_manager_connection_refused,
    server_manager_connection_request,
    server_manager_server_list_response,
    server_manager_server_not_found,
    server_manager_server_full,
    server_manager_wrong_password,
    server_manager_server_address_response,
    server_manager_server_added_to_list,
    server_manager_probe,
};