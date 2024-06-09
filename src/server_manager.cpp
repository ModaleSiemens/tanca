#include "server_manager.hpp"

#include "utils.hpp"

#include <print>
#include <iostream>

int main()
{
    ServerManager server_manager;

    std::string port;

    do
    {
        std::print("Enter the port you want to be used: ");
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
        std::bind(onClientServerListRequest, this, std::placeholders::_1, std::placeholders::_2)
    );

    client->setOnReceiving(
        Messages::client_server_address_request,
        std::bind(onClientServerAddressRequest, this, std::placeholders::_1, std::placeholders::_2)
    );    

    client->setOnReceiving(
        Messages::server_go_public,
        std::bind(onServerGoPublicRequest, this, std::placeholders::_1, std::placeholders::_2)
    );

    client->setOnReceiving(
        Messages::server_go_private,
        std::bind(onServerGoingPrivateRequest, this, std::placeholders::_1, std::placeholders::_2)
    ); 

    client->setOnReceiving(
        Messages::server_password_check_response,
        std::bind(onServerPasswordCheckResponse, this, std::placeholders::_1, std::placeholders::_2)
    );        

    client->onFailedSending = [&, this](mdsm::Collection data)
    {
        std::println("Error sending message.");
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
    std::println("Client connected while server wasn't accepting connection.");
}

void ServerManager::onClientServerListRequest(mdsm::Collection message, nets::TcpRemote<Messages>& client)
{
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

    std::println("Client asked for server - {}:{}", server_name, password);

    if(!servers_data.contains(server_name))
    {
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

        std::println("{}", server_iter != getClients().end());

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
        client.send(
            mdsm::Collection{}
                << Messages::server_manager_server_address_response
                << servers_data[server_name].address
                << servers_data[server_name].port
        );
    }
}

void ServerManager::onServerGoPublicRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    std::lock_guard lock_guard {servers_data_mutex};

    const auto server_name {message.retrieve<std::string>()};

    if(server_name.empty())
    {
        server.send(mdsm::Collection{} << Messages::server_manager_unaccepted_server_name);

        server.stop();
    }
    else if(servers_data.contains(server_name))
    {
        server.send(mdsm::Collection{} << Messages::server_manager_server_name_already_used);

        server.stop();        
    }
    else 
    {
        servers_data[server_name] = ServerData{
            server.getAddress(),
            std::to_string(server.getPort()),
            message.retrieve<bool>(),
            message.retrieve<std::size_t>(),
            message.retrieve<std::size_t>()
        };

        server.send(mdsm::Collection{} << Messages::server_manager_server_added_to_list);
    }
}

void ServerManager::onServerGoingPrivateRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
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
        server.send(mdsm::Collection{} << Messages::server_manager_server_not_found);
    }
    else 
    {
        servers_data.erase(*server_name);
    }

    server.stop();
}

void ServerManager::onServerPasswordCheckResponse(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
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
            (*client_iter)->send(
                mdsm::Collection{} << Messages::server_manager_wrong_password
            );    
        }
        else 
        {
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
    std::println("onServerPlayersCountResponse");

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
        server.send(mdsm::Collection{} << Messages::server_manager_server_not_found);

        server.stop();
    }
    else 
    {
        servers_data[*server_name].player_count = message.retrieve<std::size_t>();
    }
}

void ServerManager::updateServersData()
{
    for(auto&[name, data] : servers_data)
    {
        if(
            auto server_iter {
                std::ranges::find_if(
                    getClients(),
                    [&, this](const std::shared_ptr<nets::TcpRemote<Messages>> remote)
                    {
                        return remote->isConnected() && remote->getAddress() == data.address;
                    }
                )
            };
            server_iter != getClients().end()
        )
        {
            (*server_iter)->send(mdsm::Collection{} << Messages::server_manager_players_count_request);
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
