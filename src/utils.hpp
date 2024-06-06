#pragma once

#include <string_view>

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