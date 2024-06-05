#include "client.hpp"

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

    getWindow("main")->getWidget<tgui::Button>("by_name_button")->onClick(
        [&, this]
        {
            getWindow("main")->setTitle("Insert server data!");
            getWindow("main")->loadWidgetsFromFile("../assets/interfaces/client_by_name.txt");
        }
    );
}

void ClientApp::update(const app::Seconds elapsed_seconds)
{
    Application::update(elapsed_seconds);
}

void ClientApp::onConnection(std::shared_ptr<Remote> server)
{
}

ClientApp::~ClientApp()
{

}