#include "saves_manager.hpp"

#include <string>
#include <fstream>

SaveDataNotFound::SaveDataNotFound(const std::filesystem::path data_path)
:   
    runtime_error {
        std::format("Couldn't find save data at \"{}\".", data_path.string())   
    }
{
}

SavesDirectoryNotFound::SavesDirectoryNotFound()
:
    runtime_error {
        "Saves directory not found."   
    }
{
}

SavesManager::SavesManager(const std::filesystem::path saves_path)
:
    saves_path{saves_path}
{
    assureSavesDirectoryExist();
}

void SavesManager::assureSavesDirectoryExist()
{
    if(!std::filesystem::exists(saves_path))
    {
        throw SavesDirectoryNotFound{};
    }
}

boost::property_tree::ptree SavesManager::readInfoFromSave(const std::filesystem::path save_path)
{
    if(!std::filesystem::exists(save_path / "info.json"))
    {
        throw SaveDataNotFound{save_path};
    }
    else 
    {
        std::ifstream info_file {save_path / "info.json"};

        boost::property_tree::ptree info;

        boost::property_tree::read_json(info_file, info);

        return info;
    }
}


std::vector<SavesManager::Save> SavesManager::getSaves()
{
    std::vector<Save> saves;

    for(const auto& save_folder : std::filesystem::directory_iterator(saves_path))
    {
        using namespace std::chrono;

        auto info {readInfoFromSave(save_folder.path())};
        
        saves.emplace_back(
            info.get<std::string>("name"),
            static_cast<system_clock::time_point>(static_cast<seconds>(info.get<std::size_t>("creation_date"))),
            static_cast<system_clock::time_point>(static_cast<seconds>(info.get<std::size_t>("last_played_date"))),
            static_cast<seconds>(info.get<std::size_t>("played_time_seconds"))
        );
    }

    return saves;
}

bool SavesManager::nameIsFree(const std::string_view name)
{
    return !std::filesystem::exists(saves_path / name);
}

std::optional<std::filesystem::path> SavesManager::getSavePath(const std::string_view name)
{
    std::optional<std::filesystem::path> path {std::nullopt};

    for(const auto& save_folder : std::filesystem::directory_iterator(saves_path))
    {
        const auto info {readInfoFromSave(save_folder.path() / "info.json")};

        if(info.get<std::string>("name") == name)
        {
            path = save_folder.path();
        }
    }

    return path;
}

bool SavesManager::deleteSave(const std::string_view name)
{
    return std::filesystem::remove(saves_path / name);
}

bool SavesManager::setName(
    const std::string_view save_name,
    const std::string_view new_name
)
{
    if(nameIsFree)
    {
        return false;
    }
    else 
    {
        auto info {readInfoFromSave(*getSavePath(save_name))};

        info.put("name", new_name);

        writeInfoToSave(save_name, info);
        
        return true;
    }
}

bool SavesManager::increasePlayedTime(const std::string_view save_name, const Seconds played_time)
{
    const auto save_path {getSavePath("save_name")};
}

bool SavesManager::createSave(
    const std::string_view name,
    const std::string_view whitelist,
    const std::string_view blacklist
)
{
    if(!nameIsFree(name))
    {
        return false;
    }
    else 
    {
        std::filesystem::create_directory(saves_path / name);

        boost::property_tree::ptree info;

        info.put("name", std::string{name});
        info.put("creation_date", std::chrono::system_clock::now());
        info.put("last_playerd_date", std::chrono::system_clock::now());
        info.put("played_time_seconds", 0);
        info.put("whitelist", whitelist);
        info.put("blacklist", blacklist);

        writeInfoToSave(name, info);

        return true;
    }
}

void SavesManager::writeInfoToSave(const std::string_view save_name, const boost::property_tree::ptree& info)
{
    std::ofstream info_file {*getSavePath(save_name) / "info.json"};

    boost::property_tree::write_json(info_file, info);
}