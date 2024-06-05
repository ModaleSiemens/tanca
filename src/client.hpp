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

        virtual void onConnection(std::shared_ptr<Remote> server) override;

        virtual ~ClientApp();
};

