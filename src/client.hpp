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

}

void ClientApp::update(const app::Seconds elapsed_seconds)
{
    Application::update(elapsed_seconds);
}