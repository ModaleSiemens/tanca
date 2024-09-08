#pragma once

#include "nets.hpp"

#include "messages.hpp"

#include <unordered_map>

class Remote : public nets::TcpRemote<Messages>
{
    public:
        using TcpRemote<Messages>::TcpRemote;
};

class ServerManager : public nets::TcpServer<Messages, Remote>
{
    public:
        struct ServerData
        {
            std::string address;
            std::string port;
            std::string client_port;
            bool        requires_password;
            std::size_t player_count;
            std::size_t max_player_count;
        };

        ServerManager();

    private:
        virtual void onClientConnection         (std::shared_ptr<Remote> client) override;
        virtual void onForbiddenClientConnection(std::shared_ptr<Remote> client) override;

        void onClientServerListRequest   (mdsm::Collection message, nets::TcpRemote<Messages>& client);
        void onClientServerAddressRequest(mdsm::Collection message, nets::TcpRemote<Messages>& client);

        void onServerGoPublicRequest    (mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerGoingPrivateRequest(mdsm::Collection message, nets::TcpRemote<Messages>& server);

        void onServerPasswordCheckResponse(mdsm::Collection message, nets::TcpRemote<Messages>& server);
        void onServerPlayersCountResponse (mdsm::Collection message, nets::TcpRemote<Messages>& server);

        void updateServersData();

        bool isClient(const std::string_view address, const nets::Port port);
        bool isServer(const std::string_view address, const nets::Port port);

        std::optional<std::string> getServerNameByEndpoint(const std::string_view address, const nets::Port port);
        std::shared_ptr<Remote>    getServerByName(const std::string_view name);

        std::mutex servers_data_mutex;

        std::unordered_map<std::string, ServerData> servers_data;

        bool debug {true};
};