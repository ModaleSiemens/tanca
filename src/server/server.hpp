#pragma once

#include <map>
#include "app.hpp"
#include "nets.hpp"

#include "../messages.hpp"

#include "saves_manager.hpp"

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

        void goPublic();
        void goPrivate();

        virtual ~ServerApp();

    private:
        virtual void onConnection(std::shared_ptr<Remote> server) override;

        virtual void onClientConnection         (std::shared_ptr<Remote> client) override;  
        virtual void onForbiddenClientConnection(std::shared_ptr<Remote> client) override;

        void showEditSavePopup();

        void setupMessageCallbacks();

        void setupWelcomeInterface();
        void setupRunningInterface();

        void onServerManagerConnectionRefused     (mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);
        void onServerAddedToList                  (mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);
        void onServerRemovedFromList              (mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);
        void onServerManagerServerNotFoundResponse(mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);

        void onClientConnected(mdsm::Collection message, nets::TcpRemote<Messages>& client);
        
        void onServerManagerPasswordCheckRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);
        void onServerManagerPlayersCountRequest (mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);

        void onServerManagerNameAlreadyUsedResponse(mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);
        void onServerManagerUnacceptedNameResponse (mdsm::Collection message, nets::TcpRemote<Messages>& server_manager);

        void onClientCredentialsResponse(mdsm::Collection message, nets::TcpRemote<Messages>& client);
        
        void onClientChatMessage(mdsm::Collection message, nets::TcpRemote<Messages>& client);

        bool clientNicknameIsBanned(const std::string_view nickname);
        bool clientAddressIsBanned(const std::string_view address);
        bool validateClientCredentials(const std::string_view nickname, const std::string_view password);

        void processClient(nets::TcpRemote<Messages>& client);

        void closeServer();

        void updateSavesList();

        std::shared_ptr<app::Window> main_window;

        std::atomic_bool is_public {false};

        std::string              name;
        std::string              port;
        std::string              password;
        std::atomic<std::size_t> players_count;
        std::size_t              player_limit;

        std::string server_manager_address {""};
        std::string server_manager_port    {""};

        std::mutex interface_mutex;

        bool debug {true};

        struct PlayerData
        {
            std::shared_ptr<nets::TcpRemote<Messages>> remote;
        };

        /*
            Map is ordered to enable sorted listing of players
            The key is the nickname
        */ 
        std::map<std::string, PlayerData> players;

        std::string getPlayerNameByRemote(nets::TcpRemote<Messages>& remote);

        /*
            Constant used in the new world creation, selection and editing process
        */
        const std::string_view new_world_name {"new_world"};

        const std::filesystem::path saves_folder  {"../saves"};
        const std::filesystem::path assets_folder {"../assets"};

        SavesManager saves_manager {saves_folder};
};