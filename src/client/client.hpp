#pragma once

#include "nets.hpp"

#include "../messages.hpp"
#include "../utils.hpp"

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

        void setupMessageCallbacks();

        // Interfaces set up member functions
        void setupWelcomeInterface();
        void setupServerManagerPromptInterface();
        void setupByNamePromptInterface();
        void setupByAddressPromptInterface();

        void onServerListResponse      (mdsm::Collection servers_data, nets::TcpRemote<Messages>& server);
        void onServerNotFound          (mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerFull              (mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onWrongPassword           (mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerWrongPassword     (mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerAcceptedConnection(mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerCredentialsRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerWrongCredentials  (mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerClientIsBanned    (mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerClientIsWelcome   (mdsm::Collection message, nets::TcpRemote<Messages>& server);

        void onServerProbe(mdsm::Collection message, nets::TcpRemote<Messages>& server);

        void onServerAddressResponse(mdsm::Collection message, nets::TcpRemote<Messages>& server);

        void onConnectionRefused(mdsm::Collection message, nets::TcpRemote<Messages>& server);

        void setupConnectedToServerInterface();

        void serverListUpdater();

        //bool setBackButton(const std::string interface_path, void (ClientApp::* function)());
        std::shared_ptr<tgui::Button> getBackButton();

        std::shared_ptr<app::Window> main_window;

        enum class Status
        {
            not_connected,
            connected_to_server_manager,
            connected_to_server,
            connecting_to_server
        };

        std::atomic<Status> status;

        std::mutex internal_server_list_mutex;
        std::mutex interface_mutex;

        std::vector<ServerData> internal_server_list;
        
        std::string server_manager_address;
        std::string server_manager_port;

        bool debug {true};
};