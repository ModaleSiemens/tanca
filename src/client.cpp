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
    if(status == Status::connecting_to_server_manager)
    {
        server->setOnReceiving(
            Messages::server_list_response,
            [&, this](mdsm::Collection servers_data_collection, nets::TcpRemote<Messages>& server)
            {
                std::lock_guard<std::mutex> lock_guard {internal_server_list_mutex};
                
                internal_server_list.clear();

                while(!servers_data_collection.isEmpty())
                {
                    internal_server_list.push_back(
                        ServerData{
                            servers_data_collection.retrieve<std::string>(),
                            servers_data_collection.retrieve<bool>       (),
                            servers_data_collection.retrieve<std::size_t>(),
                            servers_data_collection.retrieve<std::size_t>()
                        }
                    );
                }
            }
        );

        server->send(mdsm::Collection{} << Messages::server_list_request);
    }
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
                std::println("Address not valid");
                main_window->addErrorToWidget(
                    "address_editbox",
                    "<color=white>Invalid address!</color>",
                    25
                );
            }
            else 
            {
                std::println("Removing error");
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
                status = Status::connecting_to_server_manager;

                setServerAddress(address);
                setServerPort(port);

                if(!connect())
                {
                    main_window->addErrorToWidget(
                        "connect_button",
                        "<color=white>Failed to connect to server manager...</color>",
                        25
                    );
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

    while(main_window->getWidget("servers_listview") != nullptr)
    {
        std::this_thread::sleep_for(PingTime{1.0});

        std::lock_guard<std::mutex> lock_guard {internal_server_list_mutex};

        auto server_listview {main_window->getWidget<tgui::ListView>("servers_listview")};

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
    }
}

std::shared_ptr<tgui::Button> ClientApp::getBackButton()
{
    return main_window->getWidget<tgui::Button>("back_button");
}

/*
bool ClientApp::setBackButton(const std::string interface_path, void (ClientApp::* function)())
{
    if(main_window->getWidget<tgui::Button>("back_button") == nullptr)
    {
        return false;
    }
    else 
    {
        main_window->getWidget<tgui::Button>("back_button")->onClick(
            [=, this]
            {
                main_window->loadWidgetsFromFile(interface_path);

                (this->*function)();
            }
        );

        return true;
    }
}
*/

ClientApp::~ClientApp()
{

}