#pragma once

#include <string_view>
#include <chrono>
#include <filesystem>
#include <optional>
#include <boost/json/src.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class SavesDirectoryNotFound : public std::runtime_error
{
    public:
        SavesDirectoryNotFound();
};

class SaveDataNotFound : public std::runtime_error
{
    public:
        explicit SaveDataNotFound(const std::filesystem::path data_path);
};

class SavesManager
{
    public:
        using Date = std::chrono::time_point<std::chrono::system_clock>; 
        using Seconds = std::chrono::duration<double>;

        struct Save 
        {
            std::string name;
            Date        creation_date;
            Date        last_played_date;
            Seconds     played_time_seconds;
            std::string whitelist;
            std::string blacklist;
        };

        SavesManager(const std::filesystem::path saves_path);

        bool setName           (const std::string_view save_name, const std::string_view new_name);
        bool increasePlayedTime(const std::string_view save_name, const Seconds played_time);

        bool createSave(
            const std::string_view save_name,
            const std::string_view whitelist,
            const std::string_view blacklist
        );

        bool deleteSave (const std::string_view save_name);

        std::vector<Save> getSaves();

    private:
        void assureSavesDirectoryExist();
        bool nameIsFree(const std::string_view name);

        std::filesystem::path saves_path;

        void                        writeInfoToSave(const std::string_view save_name, const boost::property_tree::ptree& info);
        boost::property_tree::ptree readInfoFromSave(const std::filesystem::path save_path);

        std::optional<std::filesystem::path> getSavePath(const std::string_view name);

        void createSaveDirectory(const std::string_view name);
};