#include "server_manager.hpp"

#include "utils.hpp"

#include <print>
#include <iostream>
#include <chrono>

int main()
{
    ServerManager server_manager;

    std::string port;

    do
    {
        std::print("What port number do you want me to use? ");
        std::cin >> port;
    }
    while(!isValidPort(port));

    server_manager.setIpVersion(nets::IPVersion::ipv4);
    server_manager.setPort(std::stoull(port));

    server_manager.startAccepting();

    while(true)
    {

    }
    
    return 0;
}

ServerManager::ServerManager()
{
}

void ServerManager::onClientConnection(std::shared_ptr<Remote> client)
{
    client->setOnReceiving(
        Messages::client_server_list_request,
        std::bind(&ServerManager::onClientServerListRequest, this, std::placeholders::_1, std::placeholders::_2)
    );

    client->setOnReceiving(
        Messages::client_server_address_request,
        std::bind(&ServerManager::onClientServerAddressRequest, this, std::placeholders::_1, std::placeholders::_2)
    );    

    client->setOnReceiving(
        Messages::server_go_public,
        std::bind(&ServerManager::onServerGoPublicRequest, this, std::placeholders::_1, std::placeholders::_2)
    );

    client->setOnReceiving(
        Messages::server_go_private,
        std::bind(&ServerManager::onServerGoingPrivateRequest, this, std::placeholders::_1, std::placeholders::_2)
    ); 

    client->setOnReceiving(
        Messages::server_password_check_response,
        std::bind(&ServerManager::onServerPasswordCheckResponse, this, std::placeholders::_1, std::placeholders::_2)
    );     

    client->setOnReceiving(
        Messages::server_players_count_response,
        std::bind(&ServerManager::onServerPlayersCountResponse, this, std::placeholders::_1, std::placeholders::_2)
    );               

    client->onFailedSending = [&, this](mdsm::Collection data)
    {
        if(debug)
        {
            std::println(
                "[{}]: Failed sending data.", getFormattedCurrentTime()
            );
        }
    };

    while(client->isConnected())
    {

    }

    if(
        const auto server_name {getServerNameByEndpoint(client->getAddress(), client->getPort())};
        server_name
    )
    {
        servers_data.erase(*server_name);

        closeConnection(client);
    }
}

void ServerManager::onForbiddenClientConnection(std::shared_ptr<Remote> client)
{
    if(debug)
    {
        std::println(
            "[{}]: Client shoudln't have connected... ({}:{}).", getFormattedCurrentTime(), client->getAddress(), client->getPort()
        );
    }
}

void ServerManager::onClientServerListRequest(mdsm::Collection message, nets::TcpRemote<Messages>& client)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"client_server_list_request\" ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
        );
    } 

    updateServersData();

    std::lock_guard lock_guard {servers_data_mutex}; 

    mdsm::Collection servers_data_message;

    servers_data_message << Messages::server_manager_server_list_response;

    for(const auto&[name, data] : servers_data)
    {
        servers_data_message
            << name
            << data.requires_password
            << data.player_count
            << data.max_player_count
        ;
    }

    client.send(servers_data_message);
}

void ServerManager::onClientServerAddressRequest(mdsm::Collection message, nets::TcpRemote<Messages>& client)
{
    std::lock_guard lock_guard {servers_data_mutex};

    const auto server_name {message.retrieve<std::string>()};
    const auto password    {message.retrieve<std::string>()};

    if(debug)
    {
        std::println(
            "[{}]: Received \"client_server_address_request\" ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
        );
    }

    if(!servers_data.contains(server_name))
    {
        if(debug)
        {
            std::println(
                "[{}]: Server not found ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
            );
        }

        client.send(mdsm::Collection{} << Messages::server_manager_server_not_found);
    }
    else if(servers_data[server_name].requires_password)
    {
        auto server_iter {
            std::ranges::find_if(
                getClients(),
                [&, this](const std::shared_ptr<Remote> server)
                {
                    return
                        server->getAddress() == servers_data[server_name].address
                        &&
                        std::to_string(server->getPort()) == servers_data[server_name].port
                    ;
                }
            )
        };

        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_manager_password_check_request\" ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
            );
        }

        (*server_iter)->send(
            mdsm::Collection{}
                << Messages::server_manager_password_check_request
                << password
                << client.getAddress()
                << std::to_string(client.getPort())
        );
    }
    else 
    {
        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_manager_server_address_response\" ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
            );
        }

        client.send(
            mdsm::Collection{}
                << Messages::server_manager_server_address_response
                << servers_data[server_name].address
                << servers_data[server_name].client_port
        );
    }
}

void ServerManager::onServerGoPublicRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    std::lock_guard lock_guard {servers_data_mutex};

    if(debug)
    {
        std::println(
            "[{}]: Received \"server_go_public\" request ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
        );
    }

    const auto server_name {message.retrieve<std::string>()};

    if(server_name.empty())
    {
        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_manager_unaccepted_server_name\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
            );
        }

        server.send(mdsm::Collection{} << Messages::server_manager_unaccepted_server_name);
    }
    else if(servers_data.contains(server_name))
    {
        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_manager_server_name_already_used\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
            );
        }

        server.send(mdsm::Collection{} << Messages::server_manager_server_name_already_used);    
    }
    else 
    {
        servers_data[server_name] = ServerData{
            server.getAddress(),
            std::to_string(server.getPort()),
            message.retrieve<std::string>(),
            message.retrieve<bool>(),
            message.retrieve<std::size_t>(),
            message.retrieve<std::size_t>()
        };

        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_manager_server_added_to_list\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
            );
        }        

        server.send(mdsm::Collection{} << Messages::server_manager_server_added_to_list);
    }
}

void ServerManager::onServerGoingPrivateRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    std::lock_guard lock_guard {servers_data_mutex};

    if(debug)
    {
        std::println(
            "[{}]: Received \"server_go_private\" request ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
        );
    }

    std::optional<std::string> server_name {std::nullopt};

    for(const auto&[name, data] : servers_data)
    {
        if(data.address == server.getAddress())
        {
            server_name = name;
        }
    }

    if(!server_name)
    {
        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_manager_server_not_found\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
            );
        }

        server.send(mdsm::Collection{} << Messages::server_manager_server_not_found);
    }
    else 
    {
        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_manager_server_removed_from_list\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
            );
        }

        servers_data.erase(*server_name);

        server.send(mdsm::Collection{} << Messages::server_manager_server_removed_from_list);
    }
}

void ServerManager::onServerPasswordCheckResponse(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"server_password_check_response\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
        );
    }

    const auto password_was_right {message.retrieve<bool>()};
    const auto client_address     {message.retrieve<std::string>()};
    const auto client_port        {message.retrieve<std::string>()};    
    const auto server_port        {message.retrieve<std::string>()};

    auto client_iter {
        std::ranges::find_if(
            getClients(),
            [&, this](const std::shared_ptr<Remote> client)
            {
                return
                    client->getAddress() == client_address
                    &&
                    std::to_string(client->getPort()) == client_port
                ;
            }
        )
    };

    if(client_iter != getClients().end())
    {
        if(!password_was_right)
        {
            if(debug)
            {
                std::println(
                    "[{}]: Sending \"server_manager_wrong_password\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
                );
            }

            (*client_iter)->send(
                mdsm::Collection{} << Messages::server_manager_wrong_password
            );    
        }
        else 
        {
            if(debug)
            {
                std::println(
                    "[{}]: Sending \"server_manager_server_address_response\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
                );
            }

            (*client_iter)->send(
                mdsm::Collection{}
                    << Messages::server_manager_server_address_response
                    << server.getAddress()
                    << server_port
            );
        }
    }
}

void ServerManager::onServerPlayersCountResponse(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"server_players_count_response\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
        );
    }

    std::lock_guard lock_guard {servers_data_mutex};

    std::optional<std::string> server_name {std::nullopt};

    for(const auto&[name, data] : servers_data)
    {
        if(data.address == server.getAddress())
        {
            server_name = name;
        }
    }

    if(!server_name)
    {
        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_manager_server_not_found\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
            );
        }

        server.send(mdsm::Collection{} << Messages::server_manager_server_not_found);
    }
    else 
    {
        if(debug)
        {
            std::println(
                "[{}]: Updating player count ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
            );
        }

        servers_data[*server_name].player_count = message.retrieve<std::size_t>();
    }
}

void ServerManager::updateServersData()
{
    std::lock_guard lock_guard {servers_data_mutex};

    for(auto&[name, data] : servers_data)
    {
        if(
            auto server_ptr {getServerByName(name)};
            server_ptr
        )
        {
            if(debug)
            {
                std::println(
                    "[{}]: Sending \"server_manager_players_count_request\" ({}:{}).", getFormattedCurrentTime(), data.address, data.port
                );
            }

            server_ptr->send(mdsm::Collection{} << Messages::server_manager_players_count_request);
        }
    }
}

bool ServerManager::isClient(const std::string_view address, const nets::Port port)
{
    return !isServer(address, port);
}

bool ServerManager::isServer(const std::string_view address, const nets::Port port)
{
    bool is_server {false};

    for(const auto&[name, data] : servers_data)
    {
        if(data.address == address && std::stoull(data.port) == port)
        {
            is_server = true;
            break;
        }
    }

    return is_server;
}

std::optional<std::string> ServerManager::getServerNameByEndpoint(const std::string_view address, const nets::Port port)
{
    for(const auto&[name, data] : servers_data)
    {
        if(data.address == address && std::stoull(data.port) == port)
        {
            return name;
        }
    }

    return std::nullopt;
}

std::shared_ptr<Remote> ServerManager::getServerByName(const std::string_view name)
{
    std::println("Searching for server");

    auto server_iter {
        std::ranges::find_if(
            getClients(),
            [&, this](const std::shared_ptr<Remote> remote)
            {
                return
                    remote->getAddress() == servers_data[std::string{name}].address
                    &&
                    std::to_string(remote->getPort()) == servers_data[std::string{name}].port
                ;
            } 
        )
    };

    if(server_iter != getClients().end())
    {
        return *server_iter;
    }
    else 
    {
        return nullptr;
    }
}
