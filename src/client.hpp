#pragma once

#include "app.hpp"
#include "nets.hpp"

#include "messages.hpp"

class Remote : public nets::TcpRemote<Messages>
{
    public:
        using TcpRemote<Messages>::TcpRemote;
};

class ClientApp : public app::Application, private nets::TcpClient<Messages, Remote>
{
    public:
        ClientApp();

        virtual void update(const app::Seconds elapsed_seconds) override;
        
        virtual ~ClientApp();
        
    private:
        virtual void onConnection(std::shared_ptr<Remote> server) override;

        // Interfaces set up member functions
        void setupWelcomeInterface();
        void setupServerManagerPromptInterface();
        void setupByNamePromptInterface();

        //bool setBackButton(const std::string interface_path, void (ClientApp::* function)());
        std::shared_ptr<tgui::Button> getBackButton();

        std::shared_ptr<app::Window> main_window;

        enum class Status
        {
            connecting_to_server_manager,
            connecting_to_server
        } status;

        std::mutex internal_server_list_mutex;

        std::vector<ServerData> internal_server_list;
};