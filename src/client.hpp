#pragma once

#include "app.hpp"

class ClientApp : public app::Application
{
    public:
        ClientApp();

        virtual ~ClientApp();

        virtual void update(const app::Seconds elapsed_seconds) override;
};

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
}

void ClientApp::update(const app::Seconds elapsed_seconds)
{
    Application::update(elapsed_seconds);
}

ClientApp::~ClientApp()
{

}