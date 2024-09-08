#include "server.hpp"

#include "utils.hpp"

#include <filesystem>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

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
    TcpServer{}
{
    addWindow(
        "main",
        true,
        "../assets/interfaces/server/setup.txt",
        sf::VideoMode(800, 600),
        "Welcome to tanca!"
    );

    main_window = getWindow("main");

    setupWelcomeInterface();
}

void ServerApp::update(const app::Seconds elapsed_seconds)
{
    std::lock_guard<std::mutex> lock_guard {interface_mutex};

    Application::update(elapsed_seconds);
}

void ServerApp::goPublic()
{
    if(!is_public)
    {
        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_go_public\" request ({}:{}).", getFormattedCurrentTime(), server_manager_address, server_manager_port
            );
        }

        server->send(
            mdsm::Collection{}
                << Messages::server_go_public
                << name
                << port
                << !password.empty()
                << players_count.load()
                << max_players_count
        );
    }
}

void ServerApp::goPrivate()
{
    if(is_public)
    {
        if(debug)
        {
            std::println(
                "[{}]: Sending \"server_go_private\" request ({}:{}).", getFormattedCurrentTime(), server_manager_address, server_manager_port
            );
        }

        server->send(mdsm::Collection{} << Messages::server_go_private);
    }
}

ServerApp::~ServerApp()
{
}

void ServerApp::onConnection(std::shared_ptr<Remote> server)
{
    if(debug)
    {
        std::println(
            "[{}]: Connected to Server Manager ({}:{}).", getFormattedCurrentTime(), server->getAddress(), server->getPort()
        );
    }

    server->setOnReceiving(
        Messages::server_manager_connection_refused,
        std::bind(&ServerApp::onServerManagerConnectionRefused, this, std::placeholders::_1, std::placeholders::_2)
    );

    server->setOnReceiving(
        Messages::server_manager_server_added_to_list,
        std::bind(&ServerApp::onServerAddedToList, this, std::placeholders::_1, std::placeholders::_2)
    );

    server->setOnReceiving(
        Messages::server_manager_server_removed_from_list,
        std::bind(&ServerApp::onServerRemovedFromList, this, std::placeholders::_1, std::placeholders::_2)
    );    

    server->setOnReceiving(
        Messages::server_manager_password_check_request,
        std::bind(&ServerApp::onServerManagerPasswordCheckRequest, this, std::placeholders::_1, std::placeholders::_2)
    );

    server->setOnReceiving(
        Messages::server_manager_unaccepted_server_name,
        std::bind(&ServerApp::onServerManagerUnacceptedNameResponse, this, std::placeholders::_1, std::placeholders::_2)
    );    

    server->setOnReceiving(
        Messages::server_manager_server_name_already_used,
        std::bind(&ServerApp::onServerManagerNameAlreadyUsedResponse, this, std::placeholders::_1, std::placeholders::_2)
    );    

    server->setOnReceiving(
        Messages::server_manager_server_not_found,
        std::bind(&ServerApp::onServerManagerServerNotFoundResponse, this, std::placeholders::_1, std::placeholders::_2)
    );       

    server->setOnReceiving(
        Messages::server_manager_players_count_request,
        std::bind(&ServerApp::onServerManagerPlayersCountRequest, this, std::placeholders::_1, std::placeholders::_2)
    );       

    server->onFailedSending = [&, this](mdsm::Collection data)
    {
        if(debug)
        {
            std::println(
                "[{}]: Failed sending data.", getFormattedCurrentTime()
            );
        }
    };

    server->onFailedReading = [&, this](std::optional<boost::system::error_code> error)
    {
        if(debug)
        {
            std::println(
                "[{}]: Failed reading data.", getFormattedCurrentTime()
            );
        }
    };  

    goPublic();

    while(true)
    {

    }
}

void ServerApp::onClientConnection(std::shared_ptr<Remote> client)
{
    if(debug)
    {
        std::println(
            "[{}]: Client connected ({}:{}).", getFormattedCurrentTime(), client->getAddress(), client->getPort()
        );
    }

    client->setOnReceiving(
        Messages::client_connection_request,
        std::bind(&ServerApp::onClientConnected, this, std::placeholders::_1, std::placeholders::_2)
    );

    client->setOnReceiving(
        Messages::client_credentials_response,
        std::bind(&ServerApp::onClientCredentialsResponse, this, std::placeholders::_1, std::placeholders::_2)
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

    client->onFailedReading = [&, this](std::optional<boost::system::error_code> error)
    {
        if(debug)
        {
            std::println(
                "[{}]: Failed reading data.", getFormattedCurrentTime()
            );
        }
    };       
}

void ServerApp::onForbiddenClientConnection(std::shared_ptr<Remote> client)
{
    if(debug)
    {
        std::println(
            "[{}]: Client shouldn't have connected... ({}:{}).", getFormattedCurrentTime(), client->getAddress(), client->getPort()
        );
    }
}

void ServerApp::setupRunningInterface()
{
    main_window->loadWidgetsFromFile("../assets/interfaces/server/running.txt");

    main_window->getWidget<tgui::ToggleButton>("public_toggle_button")->onToggle(
        [&, this]
        {
            if(is_public)
            {   
                goPrivate();
            }
            else 
            {
                goPublic();
            }
        }
    );
}

void ServerApp::setupWelcomeInterface()
{
    main_window->loadWidgetsFromFile("../assets/interfaces/server/setup.txt");

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
            const auto server_password         {main_window->getWidget<tgui::EditBox>("password_editbox")->getText().toStdString()};
            const auto server_max_player_count {main_window->getWidget<tgui::EditBox>("max_player_count_editbox")->getText().toStdString()};  

            port = main_window->getWidget<tgui::EditBox>("port_editbox")->getText().toStdString();

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
                //setupRunningInterface();

                setIpVersion(nets::IPVersion::ipv4);
                setPort(std::stoull(port));

                password = server_password;
                max_players_count = server_max_player_count == "" ? 0 : std::stoull(server_max_player_count);
                players_count = 0;

                startAccepting();

                if(main_window->getWidget<tgui::CheckBox>("make_public_checkbox")->isChecked())
                {
                    const auto server_name {main_window->getWidget<tgui::EditBox>("name_editbox")->getText().toStdString()};
                    
                    server_manager_address = main_window->getWidget<tgui::EditBox>("server_manager_address_editbox")->getText().toStdString();
                    server_manager_port    = main_window->getWidget<tgui::EditBox>("server_manager_port_editbox")->getText().toStdString();                             

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

                        name = server_name;

                        if(connect())
                        {
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

                setupRunningInterface();
            }

        }
    );
}

void ServerApp::onServerManagerConnectionRefused(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"server_manager_connection_refused\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
        );
    }
}

void ServerApp::onServerAddedToList(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    is_public = true;

    if(auto public_toggle {main_window->getWidget<tgui::ToggleButton>("public_toggle_button")})
    {
        public_toggle->setDown(true);
    }

    if(debug)
    {
        std::println(
            "[{}]: Received \"server_manager_server_added_to_list\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
        );
    }
}

void ServerApp::onServerRemovedFromList(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    is_public = false;

    if(auto public_toggle {main_window->getWidget<tgui::ToggleButton>("public_toggle_button")})
    {
        public_toggle->setDown(false);
    }    

    if(debug)
    {
        std::println(
            "[{}]: Received \"server_manager_server_removed_from_list\" ({}:{}).", getFormattedCurrentTime(), server.getAddress(), server.getPort()
        );
    }
}

void ServerApp::onServerManagerServerNotFoundResponse(mdsm::Collection message, nets::TcpRemote<Messages> &server_manager)
{   
    is_public = false;

    if(debug)
    {
        std::println(
            "[{}]: Received \"server_manager_server_not_found\" ({}:{}).", getFormattedCurrentTime(), server_manager.getAddress(), server_manager.getPort()
        );
    }
}

void ServerApp::onClientConnected(mdsm::Collection message, nets::TcpRemote<Messages>& client)
{
    if(!password.empty())
    {
        if(message.retrieve<std::string>() != password)
        {   
            if(debug)
            {
                std::println(
                    "[{}]: Received password is wrong ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
                );
            }

            client.send(mdsm::Collection{} << Messages::server_wrong_password);

            client.stop();

            return;
        }
    }

    if((players_count.load() + 1) <= max_players_count || max_players_count <= 0)
    {
        // Client connected, now ask for credentials

        client.send(
            mdsm::Collection{}
                << Messages::server_credentials_request
        );

        /*
        std::lock_guard<std::mutex> lock_guard {interface_mutex};

        ++players_count;

        client.send(mdsm::Collection{} << Messages::server_connection_accepted);

        while(client.isConnected())
        {
            if(debug)
            {
                std::println(
                    "[{}]: Probing client ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
                );
            }

            client.send(mdsm::Collection{} << Messages::server_probe);

            std::this_thread::sleep_for(TcpClient::PingTime{2});
        }

        --players_count;

        if(debug)
        {
            std::println(
                "[{}]: Client disconnected ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
            );
        }
        */
    }
    else 
    {
        client.send(mdsm::Collection{} << Messages::server_full);

        if(debug)
        {
            std::println(
                "[{}]: Server is full ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
            );
        }

        client.stop();

        return;
    }
}

void ServerApp::onServerManagerPasswordCheckRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server_manager)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"server_manager_password_check_request\" ({}:{}).", getFormattedCurrentTime(), server_manager.getAddress(), server_manager.getPort()
        );
    }    

    server_manager.send(
        mdsm::Collection{}
            << Messages::server_password_check_response
            << (message.retrieve<std::string>() == password)
            << message.retrieve<std::string>()
            << message.retrieve<std::string>()
            << port
    );
}

void ServerApp::onServerManagerPlayersCountRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server_manager)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"server_manager_player_count_request\" ({}:{}).", getFormattedCurrentTime(), server_manager.getAddress(), server_manager.getPort()
        );
    }

    server_manager.send(mdsm::Collection{} << Messages::server_players_count_response << players_count.load());
}

void ServerApp::onServerManagerNameAlreadyUsedResponse(mdsm::Collection message, nets::TcpRemote<Messages> &server_manager)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"server_manager_server_name_already_used\" ({}:{}).", getFormattedCurrentTime(), server_manager.getAddress(), server_manager.getPort()
        );
    }    
}

void ServerApp::onServerManagerUnacceptedNameResponse(mdsm::Collection message, nets::TcpRemote<Messages> &server_manager)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"server_manager_unaccepted_server_name\" ({}:{}).", getFormattedCurrentTime(), server_manager.getAddress(), server_manager.getPort()
        );
    }    
}

void ServerApp::onClientCredentialsResponse(mdsm::Collection message, nets::TcpRemote<Messages> &client)
{
    if(debug)
    {
        std::println(
            "[{}]: Received \"client_credentials_response\" ({}:{}).", getFormattedCurrentTime(), client.getAddress(), client.getPort()
        );
    } 

    const auto nickname {message.retrieve<std::string>()};
    const auto password {message.retrieve<std::string>()};

    println("Name: {}, password: {}", nickname, password);
}
