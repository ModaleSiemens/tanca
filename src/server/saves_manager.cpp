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

std::optional<SavesManager::Save> SavesManager::getSave(const std::string_view name)
{
    using namespace std::chrono;

    const auto save_path {getSavePath(name)};

    if(!save_path)
    {
        return std::nullopt;
    }
    else 
    {
        const auto info {readInfoFromSave(*getSavePath(name))};

        return {
            Save {
                info.get<std::string>("name"),
                info.get<std::string>("creation_date"),
                info.get<std::string>("last_played_date"),
                info.get<std::size_t>("played_time_seconds"),
                info.get<std::string>("whitelist"),
                info.get<std::string>("blacklist")
            }
        };
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
            info.get<std::string>("creation_date"),
            info.get<std::string>("last_played_date"),
            info.get<size_t>("played_time_seconds"),
            info.get<std::string>("whitelist"),
            info.get<std::string>("blacklist")
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
        const auto info {readInfoFromSave(save_folder.path())};

        if(info.get<std::string>("name") == name)
        {
            path = save_folder.path();
        }
    }

    return path;
}

std::string SavesManager::generateSaveDirectoryName(const std::string_view save_name)
{
    auto name {std::string{save_name}};

    std::for_each(
        name.begin(),
        name.end(),
        [&](char& c)
        {
            c = std::isalnum(c) ? c : '_';
        } 
    );

    if(std::filesystem::exists(saves_path / name))
    {
        std::size_t largest_save_index {0};        

        for(const auto& save : std::filesystem::directory_iterator{saves_path})
        {
            const auto current_save_name {save.path().filename().string()};

            if(
                const auto dash_pos {current_save_name.rfind('-')};
                dash_pos != std::string::npos
            )
            {
                if(current_save_name.substr(0, dash_pos) == name)
                {
                    const auto save_index {
                        std::stoull(
                            current_save_name.substr(
                                dash_pos,
                                std::string::npos
                            )
                        )
                    };

                    if(save_index > largest_save_index)
                    {
                        largest_save_index = save_index;
                    }
                }
            }
        }

        name += std::format("-{}", largest_save_index + 1);
    }

    return name;
}

bool SavesManager::saveExists(const std::string_view name)
{
    return getSavePath(name).has_value();
}

bool SavesManager::deleteSave(const std::string_view name)
{
    const auto path {getSavePath(name)};

    if(!path)
    {
        return false;
    }
    else 
    {
        std::filesystem::remove_all(*getSavePath(name));

        return true;
    }
}

bool SavesManager::setName(
    const std::string_view save_name,
    const std::string_view new_name
)
{
    if(!saveExists(save_name))
    {
        return false;
    }
    else 
    {
        auto info {readInfoFromSave(*getSavePath(save_name))};

        info.put("name", new_name);

        writeInfoToSave(*getSavePath(save_name), info);
        
        return true;
    }
}

bool SavesManager::setWhitelist(const std::string_view save_name, const std::string_view whitelist)
{
    if(!saveExists(save_name))
    {
        return false;
    }
    else 
    {
        auto info {readInfoFromSave(*getSavePath(save_name))};

        info.put("whitelist", whitelist);

        writeInfoToSave(*getSavePath(save_name), info);
        
        return true;
    }
}

bool SavesManager::setBlacklist(const std::string_view save_name, const std::string_view blacklist)
{
    if(!saveExists(save_name))
    {
        return false;
    }
    else 
    {
        auto info {readInfoFromSave(*getSavePath(save_name))};

        info.put("blacklist", blacklist);

        writeInfoToSave(*getSavePath(save_name), info);
        
        return true;
    }
}

bool SavesManager::increasePlayedTime(const std::string_view save_name, const Seconds played_time)
{
    if(!saveExists(save_name))
    {
        return false;   
    }
    else 
    {
        auto info {readInfoFromSave(*getSavePath(save_name))};

        info.put("played_time_seconds", info.get<std::size_t>("played_time_seconds") + played_time.count());

        writeInfoToSave(*getSavePath(save_name), info);
    
        return true;
    }
}

bool SavesManager::createSave(
    const std::string_view name,
    const std::string_view whitelist,
    const std::string_view blacklist
)
{
    if(saveExists(name))
    {
        return false;
    }
    else 
    {
        const auto dir_name {generateSaveDirectoryName(name)};

        std::filesystem::create_directory(saves_path / dir_name);

        boost::property_tree::ptree info;

        info.put("name", std::string{name});
        info.put("creation_date", std::chrono::system_clock::now());
        info.put("last_played_date", std::chrono::system_clock::now());
        info.put("played_time_seconds", 0);
        info.put("whitelist", whitelist);
        info.put("blacklist", blacklist);

        writeInfoToSave(saves_path / dir_name, info);

        return true;
    }
}

void SavesManager::writeInfoToSave(const std::filesystem::path save_path, const boost::property_tree::ptree& info)
{
    std::ofstream info_file {save_path / "info.json"};

    boost::property_tree::write_json(info_file, info);
}