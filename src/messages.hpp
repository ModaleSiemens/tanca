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
    client_server_address_request,

    // Messages sent from server
    server_connection_refused,
    server_password_check_response,
    server_probe,
    server_full,
    server_go_public,
    server_go_private,
    server_players_count_response,

    // Messages sent from server manager
    server_manager_connection_refused,
    server_manager_server_list_response,
    server_manager_server_not_found,
    server_manager_server_full,
    server_manager_wrong_password,
    server_manager_server_address_response,
    server_manager_server_added_to_list,
    server_manager_probe,
    server_manager_password_check_request,
    server_manager_server_name_already_used,
    server_manager_unaccepted_server_name,
    server_manager_players_count_request,
};