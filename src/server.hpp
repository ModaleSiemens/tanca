#pragma once

#include "app.hpp"
#include "nets.hpp"

#include "messages.hpp"

class Remote : public nets::TcpRemote<Messages>
{
    public:
        using TcpRemote<Messages>::TcpRemote;
};

class ServerApp
:
    public app::Application,
    private virtual nets::TcpServer<Messages, Remote>,
    private virtual nets::TcpClient<Messages, Remote>
{
    public:
        ServerApp();

        virtual void update(const app::Seconds elapsed_seconds) override;

        virtual ~ServerApp();

    private:
        virtual void onConnection(std::shared_ptr<Remote> server) override;

        virtual void onClientConnection         (std::shared_ptr<Remote> client) override;  
        virtual void onForbiddenClientConnection(std::shared_ptr<Remote> client) override;

        void setupMessageCallbacks();

        void setupWelcomeInterface();
        void setupRunningInterface();

        void onConnectionServerManagerRefused(mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerAddedToList             (mdsm::Collection message, nets::TcpRemote<Messages>& server);

        void onClientConnected       (mdsm::Collection message, nets::TcpRemote<Messages>& client);
        void onServerManagerConnected(mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);

        std::shared_ptr<app::Window> main_window;

        bool make_public;
};