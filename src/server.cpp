#include "server.hpp"

#include "utils.hpp"

#include <filesystem>
#include <thread>

int main()
{
    ServerApp server_app;

    while(true)
    {
        server_app.update({});
    }

    return 0;
}

ServerApp::ServerApp()
:
    TcpServer{
        
    }
{
    setupMessageCallbacks();

    addWindow(
        "main",
        true,
        "../assets/interfaces/server1.txt",
        sf::VideoMode(800, 600),
        "Welcome to tanca!"
    );

    main_window = getWindow("main");

    setupWelcomeInterface();
}

void ServerApp::update(const app::Seconds elapsed_seconds)
{
    Application::update(elapsed_seconds);
}

void ServerApp::goPublic()
{
    if(!is_public)
    {
        server->send(
            mdsm::Collection{}
                << Messages::server_go_public
                << name
                << !password.empty()
                << players_count.load()
                << max_players_count
        );

        std::println("Sent go_public");
    }
}

void ServerApp::goPrivate()
{
    if(is_public)
    {
        server->send(mdsm::Collection{} << Messages::server_go_private);
    }
}

ServerApp::~ServerApp()
{
}

void ServerApp::onConnection(std::shared_ptr<Remote> server)
{
    // Connection remains open until server goes private

    server->setOnReceiving(
        Messages::server_manager_connection_refused,
        std::bind(onServerManagerConnectionRefused, this, std::placeholders::_1, std::placeholders::_2)
    );

    server->setOnReceiving(
        Messages::server_manager_server_added_to_list,
        std::bind(onServerAddedToList, this, std::placeholders::_1, std::placeholders::_2)
    );

    server->setOnReceiving(
        Messages::server_manager_password_check_request,
        std::bind(onServerManagerPasswordCheckRequest, this, std::placeholders::_1, std::placeholders::_2)
    );

    server->setOnReceiving(
        Messages::server_manager_unaccepted_server_name,
        std::bind(onServerManagerUnacceptedNameResponse, this, std::placeholders::_1, std::placeholders::_2)
    );    

    server->setOnReceiving(
        Messages::server_manager_server_name_already_used,
        std::bind(onServerManagerNameAlreadyUsedResponse, this, std::placeholders::_1, std::placeholders::_2)
    );    

    server->setOnReceiving(
        Messages::server_manager_server_not_found,
        std::bind(onServerManagerServerNotFoundResponse, this, std::placeholders::_1, std::placeholders::_2)
    );        

    goPublic();

    while(server->isConnected())
    {
        
    }

    server->stop();
}

void ServerApp::onClientConnection(std::shared_ptr<Remote> client)
{
    client->setOnReceiving(
        Messages::client_connection_request,
        std::bind(onClientConnected, this, std::placeholders::_1, std::placeholders::_2)
    );
}

void ServerApp::onForbiddenClientConnection(std::shared_ptr<Remote> client)
{
    std::println("Client forbiddenly connected!");
}

void ServerApp::setupMessageCallbacks()
{
}

void ServerApp::setupWelcomeInterface()
{
    main_window->loadWidgetsFromFile("../assets/interfaces/server1.txt");

    main_window->getWidget<tgui::CheckBox>("make_public_checkbox")->onCheck(
        [&, this]
        {
            main_window->getWidget("name_editbox")->setVisible(true);
            main_window->getWidget("server_manager_address_editbox")->setVisible(true);
            main_window->getWidget("server_manager_port_editbox")->setVisible(true);
        }
    );

    main_window->getWidget<tgui::CheckBox>("make_public_checkbox")->onUncheck(
        [&, this]
        {
            main_window->getWidget("name_editbox")->setVisible(false);
            main_window->getWidget("server_manager_address_editbox")->setVisible(false);
            main_window->getWidget("server_manager_port_editbox")->setVisible(false);

            main_window->removeErrorFromWidget("name_editbox");
            main_window->removeErrorFromWidget("server_manager_address_editbox");
            main_window->removeErrorFromWidget("server_manager_port_editbox");
        }
    );    

    main_window->getWidget<tgui::Button>("start_button")->onClick(
        [&, this]
        {
            const auto world_name              {main_window->getWidget<tgui::EditBox>("world_name_editbox")->getText().toStdString()};
            const auto port                    {main_window->getWidget<tgui::EditBox>("port_editbox")->getText().toStdString()};
            const auto server_password         {main_window->getWidget<tgui::EditBox>("password_editbox")->getText().toStdString()};
            const auto server_max_player_count {main_window->getWidget<tgui::EditBox>("max_player_count_editbox")->getText().toStdString()};  

            const bool world_exists {
                std::filesystem::exists(std::string{"../data/worlds/"} + world_name) && !world_name.empty()
            };

            if(!world_exists)
            {
                main_window->addErrorToWidget(
                    "world_name_editbox",
                    "<color=white>World not found!</color>",
                    20,
                    25
                );
            }
            else 
            {
                main_window->removeErrorFromWidget("world_name_editbox");
            }

            if(port.empty())
            {
                main_window->addErrorToWidget(
                    "port_editbox",
                    "<color=white>Port can't be empty!</color>",
                    20,
                    25
                );
            }
            else 
            {
                main_window->removeErrorFromWidget("port_editbox");
            } 

            if(world_exists && !port.empty())
            {
                setupRunningInterface();

                if(main_window->getWidget<tgui::CheckBox>("make_public_checkbox")->isChecked())
                {
                    const auto server_name            {main_window->getWidget<tgui::EditBox>("name_editbox")->getText().toStdString()};
                    const auto server_manager_address {main_window->getWidget<tgui::EditBox>("server_manager_address_editbox")->getText().toStdString()};
                    const auto server_manager_port    {main_window->getWidget<tgui::EditBox>("server_manager_port_editbox")->getText().toStdString()};                             

                    if(server_name.empty())
                    {
                        main_window->addErrorToWidget(
                            "name_editbox",
                            "<color=white>Server name can't be empty!</color>",
                            20
                        );
                    }
                    else 
                    {
                        main_window->removeErrorFromWidget("name_editbox");
                    }

                    if(server_manager_port.empty())
                    {
                        main_window->addErrorToWidget(
                            "server_manager_port_editbox",
                            "<color=white>Server manager port\ncan't be empty!</color>",
                            20,
                            50
                        );
                    }
                    else 
                    {
                        main_window->removeErrorFromWidget("server_manager_port_editbox");
                    }                

                    if(server_manager_address.empty())
                    {
                        main_window->addErrorToWidget(
                            "server_manager_address_editbox",
                            "<color=white>Server manager address\ncan't be empty!</color>",
                            20,
                            50
                        );
                    }
                    else if(!isValidAddress(server_manager_address))
                    {
                        main_window->addErrorToWidget(
                            "server_manager_address_editbox",
                            "<color=white>Address not valid!</color>",
                            20
                        );
                    }
                    else 
                    {
                        main_window->removeErrorFromWidget("server_manager_address_editbox");
                    }

                    if(
                        isValidAddress(server_manager_address)
                        &&
                        !server_manager_port.empty()
                        &&
                        !server_name.empty()
                    )
                    {
                        setServerAddress(server_manager_address);
                        setServerPort   (server_manager_port);

                        name     = server_name;
                        password = server_password;
                        max_players_count = server_max_player_count == "" ? 0 : std::stoull(server_max_player_count);
                        players_count = 0;

                        std::println("{}, {}", server_manager_address, server_manager_port);

                        if(connect())
                        {
                            std::println("Connected");
                        }
                        else 
                        {
                            main_window->addErrorToWidget(
                                "start_button",
                                "<color=white>Failed to connect\nto server manager...</color>",
                                20,
                                50
                            );
                        }
                    }
                }                
            }

        }
    );
}

void ServerApp::setupRunningInterface()
{
}

void ServerApp::onServerManagerConnectionRefused(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    std::println("Server Manager refused connection!");
}

void ServerApp::onServerAddedToList(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    is_public = true;

    std::println("Server successfully added to list!");
}

void ServerApp::onServerManagerServerNotFoundResponse(mdsm::Collection message, nets::TcpRemote<Messages> &server_manager)
{   
    is_public = false;

    std::println("Server wasn't found on Server Manager!");
}

void ServerApp::onClientConnected(mdsm::Collection message, nets::TcpRemote<Messages>& client)
{
    if(!password.empty())
    {
        if(message.retrieve<std::string>() != password)
        {
            client.send(mdsm::Collection{} << Messages::server_wrong_password);

            client.stop();

            return;
        }
    }

    if((players_count.load() + 1) <= max_players_count)
    {
        client.send(mdsm::Collection{} << Messages::server_connection_accepted);

        while(client.isConnected())
        {
            client.send(mdsm::Collection{} << Messages::server_probe);

            std::this_thread::sleep_for(TcpClient::PingTime{1});
        }
    }
}

void ServerApp::onServerManagerPasswordCheckRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server_manager)
{
    if(message.retrieve<std::string>() != password)
    {
        server_manager.send(
            mdsm::Collection{}
                << Messages::server_password_check_response
                << false
                << message.retrieve<std::string>()
        );
    }
    else 
    {
        server_manager.send(
            mdsm::Collection{}
                << Messages::server_password_check_response
                << true
                << message.retrieve<std::string>()
        );
    }
}

void ServerApp::onServerManagerPlayersCountRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server_manager)
{
    server_manager.send(mdsm::Collection{} << Messages::server_players_count_response << players_count.load());
}

void ServerApp::onServerManagerNameAlreadyUsedResponse(mdsm::Collection message, nets::TcpRemote<Messages> &server_manager)
{
}

void ServerApp::onServerManagerUnacceptedNameResponse(mdsm::Collection message, nets::TcpRemote<Messages> &server_manager)
{
}
