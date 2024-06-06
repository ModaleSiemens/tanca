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
    ping_request,
    ping_response,
    server_list_request,
    server_list_response
};