#pragma once

#include <string_view>
#include <format>
#include <chrono>
#include <boost/asio.hpp>

bool isValidIPv4Address(const std::string_view address)
{
    boost::system::error_code error;

    boost::asio::ip::make_address_v4(address, error);

    return !error;
}

bool isValidIPv6Address(const std::string_view address)
{
    boost::system::error_code error;

    boost::asio::ip::make_address_v6(address, error);

    return !error;
}

bool isValidAddress(const std::string_view address)
{
    return isValidIPv4Address(address) || isValidIPv6Address(address);
}

bool isValidPort(const std::size_t port)
{
    return port >= 1 && port <= 65535;
}

bool isValidPort(const std::string_view port)
{
    try
    {
        return isValidPort(std::stoull(std::string{port}));
    }
    catch(const std::exception& e)
    {
        return false;
    }
}

std::string getFormattedCurrentTime()
{
    return std::format(
        "{:%Y-%m-%d %X}",
        std::chrono::current_zone()->to_local(std::chrono::system_clock::now())
    );
}