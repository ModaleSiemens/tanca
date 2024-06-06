#include "client.hpp"
#include "utils.hpp"

#include <print>

int main()
{
    ClientApp client_app;

    while(true)
    {
        client_app.update({});
    }

    return 0;
}

ClientApp::ClientApp()
{
    setupMessageCallbacks();

    // Add main window
    addWindow(
        "main",
        true,
        "../assets/interfaces/client1.txt",
        sf::VideoMode(800, 600),
        "Welcome to tanca!"
    );

    main_window = getWindow("main");

    setupWelcomeInterface();
}

void ClientApp::update(const app::Seconds elapsed_seconds)
{
    Application::update(elapsed_seconds);
}

void ClientApp::onConnection(std::shared_ptr<Remote> server)
{
}

void ClientApp::setupMessageCallbacks()
{
    server->setOnReceiving(
        Messages::server_list_response,
        std::bind(onServerListResponse, this, std::placeholders::_1, std::placeholders::_2)
    );

    server->setOnReceiving(
        Messages::server_not_found,
        std::bind(onServerNotFound, this, std::placeholders::_1, std::placeholders::_2)
    );

    server->setOnReceiving(
        Messages::server_full,
        std::bind(onServerFull, this, std::placeholders::_1, std::placeholders::_2)
    );    

    server->setOnReceiving(
        Messages::wrong_password,
        std::bind(onWrongPassword, this, std::placeholders::_1, std::placeholders::_2)
    );    

    server->setOnReceiving(
        Messages::connection_refused,
        std::bind(onConnectionRefused, this, std::placeholders::_1, std::placeholders::_2)
    );    

    server->setOnReceiving(
        Messages::server_address_response,
        std::bind(onServerAddressResponse, this, std::placeholders::_1, std::placeholders::_2)
    );    
}

void ClientApp::setupWelcomeInterface()
{   
    main_window->loadWidgetsFromFile("../assets/interfaces/client1.txt");

    main_window->getWidget<tgui::Button>("by_name_button")->onClick(
        [&, this]
        {
            setupServerManagerPromptInterface();
        }
    );
}

void ClientApp::setupServerManagerPromptInterface()
{
    main_window->setTitle("Enter server manager data!");
    main_window->loadWidgetsFromFile("../assets/interfaces/client_server_manager.txt");

    getBackButton()->onClick(
        [&, this]
        {
            setupWelcomeInterface();
        }
    );

    main_window->getWidget<tgui::Button>("connect_button")->onClick(
        [&, this]
        {
            main_window->removeErrorFromWidget("connect_button");

            const std::string address {
                main_window->getWidget<tgui::EditBox>("address_editbox")->getText().toStdString()
            };

            const std::string port {
                main_window->getWidget<tgui::EditBox>("port_editbox")->getText().toStdString()
            };            

            if(address.empty())
            {
                main_window->addErrorToWidget(
                    "address_editbox",
                    "<color=white>Address can't be empty!</color>",
                    25
                );
            }
            else if(!isValidAddress(address))
            {
                main_window->addErrorToWidget(
                    "address_editbox",
                    "<color=white>Invalid address!</color>",
                    25
                );
            }
            else 
            {
                main_window->removeErrorFromWidget("address_editbox");
            }

            if(port.empty())
            {
                main_window->addErrorToWidget(
                    "port_editbox",
                    "<color=white>Port number can't be empty!</color>",
                    25
                );
            }        
            else 
            {
                main_window->removeErrorFromWidget("port_editbox");
            }    
            
            if(isValidAddress(address) && !port.empty())
            {
                setServerAddress(server_manager_address = address);
                setServerPort   (server_manager_port    = port);

                if(!connect())
                {
                    main_window->addErrorToWidget(
                        "connect_button",
                        "<color=white>Failed to connect to server manager...</color>",
                        25
                    );
                }
                else 
                {
                    status = Status::connected_to_server_manager;
                    
                    setupByNamePromptInterface();
                }
            } 
        }
    );
}

void ClientApp::setupByNamePromptInterface()
{
    main_window->setTitle("Insert server data!");
    main_window->loadWidgetsFromFile("../assets/interfaces/client_by_name.txt"); 

    getBackButton()->onClick(
        [&, this]
        {
            setupServerManagerPromptInterface();
        }
    );

    main_window->getWidget<tgui::Button>("connect_button")->onClick(
        [&, this]
        {
            if(
                const std::string name {main_window->getWidget<tgui::EditBox>("name_editbox")->getText().toStdString()};
                name != ""
            )
            {
                main_window->removeErrorFromWidget("connect_button");

                server->send(
                    mdsm::Collection{}
                        << Messages::connection_request
                        << name
                        << main_window->getWidget<tgui::EditBox>("password_editbox")->getText().toStdString()
                );
            }
            else 
            {
                main_window->addErrorToWidget(
                    "name_editbox",
                    "<color=white>Server name can't be empty!</color>",
                    25
                );
            }
        }
    );

    while(main_window->getWidget("servers_listview") != nullptr)
    {
        if(status.load() == Status::connected_to_server_manager)
        {
            server->send(mdsm::Collection{} << Messages::server_list_request);

            auto server_listview {main_window->getWidget<tgui::ListView>("servers_listview")};

            std::lock_guard<std::mutex> lock_guard {internal_server_list_mutex};

            for(const auto& server_data : internal_server_list)
            {
                server_listview->addItem(
                    std::vector<tgui::String>
                    {
                        server_data.name,
                        server_data.password_required ? "Yes" : "No",
                        std::to_string(server_data.players_count),
                        server_data.max_player_count > 0 ? std::to_string(server_data.max_player_count) : "No limit"
                    }
                );
            }

            std::this_thread::sleep_for(PingTime{1.0});
        }
    }
}

void ClientApp::onServerListResponse(mdsm::Collection servers_data, nets::TcpRemote<Messages> &server)
{
    std::lock_guard<std::mutex> lock_guard {internal_server_list_mutex};
    
    internal_server_list.clear();

    while(!servers_data.isEmpty())
    {
        internal_server_list.push_back(
            ServerData{
                servers_data.retrieve<std::string>(),
                servers_data.retrieve<bool>       (),
                servers_data.retrieve<std::size_t>(),
                servers_data.retrieve<std::size_t>()
            }
        );
    }    
}

void ClientApp::onServerNotFound(mdsm::Collection message, nets::TcpRemote<Messages> &server)
{
    main_window->addErrorToWidget(
        "connect_button",
        "<color=white>Server not found!</color>",
        25
    );
}

void ClientApp::onServerFull(mdsm::Collection message, nets::TcpRemote<Messages> &server)
{
    main_window->addErrorToWidget(
        "connect_button",
        "<color=white>Server is full!</color>",
        25
    );
}

void ClientApp::onWrongPassword(mdsm::Collection message, nets::TcpRemote<Messages> &server)
{
    main_window->addErrorToWidget(
        "connect_button",
        "<color=white>Password is wrong!</color>",
        25
    );
}

void ClientApp::onConnectionRefused(mdsm::Collection message, nets::TcpRemote<Messages> &server)
{
    main_window->addErrorToWidget(
        "connect_button",
        std::string{"<color=white>Connection refused!\n</color>"} + message.retrieve<std::string>(),
        25
    );
}

void ClientApp::setupConnectedToServerInterface()
{
    main_window->setTitle("Get ready to play!");
    main_window->loadWidgetsFromFile("../assets/interfaces/client_connected");
}

void ClientApp::onServerAddressResponse(mdsm::Collection message, nets::TcpRemote<Messages>& server)
{
    disconnect();

    status = Status::not_connected;

    setServerAddress(message.retrieve<std::string>());
    setServerPort   (message.retrieve<std::string>());

    if(connect())
    {
        status = Status::connected_to_server;

        setupConnectedToServerInterface();
    }
    else
    {
        main_window->addErrorToWidget(
            "connect_button",
            std::string{"<color=white>Failed to connect to server!\n</color>"} + message.retrieve<std::string>(),
            25
        );

        setServerAddress(server_manager_address);
        setServerPort   (server_manager_port);        

        if(!connect())
        {
            setupServerManagerPromptInterface();
        }
        else 
        {
            status = Status::connected_to_server_manager;
        }
    }
}

std::shared_ptr<tgui::Button> ClientApp::getBackButton()
{
    return main_window->getWidget<tgui::Button>("back_button");
}

ClientApp::~ClientApp()
{

}