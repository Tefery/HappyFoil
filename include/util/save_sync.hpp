#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "shopInstall.hpp"

namespace inst::save_sync {
    struct SaveSyncEntry {
        std::uint64_t titleId = 0;
        std::string titleName;
        bool localAvailable = false;
        bool remoteAvailable = false;
        std::string remoteDownloadUrl;
        std::uint64_t remoteSize = 0;
    };

    bool FetchRemoteSaveItems(const std::string& shopUrl, const std::string& user, const std::string& pass, std::vector<shopInstStuff::ShopItem>& outItems, std::string& warning);
    bool BuildEntries(const std::vector<shopInstStuff::ShopItem>& remoteItems, std::vector<SaveSyncEntry>& outEntries, std::string& warning);
    bool UploadSaveToServer(const std::string& shopUrl, const std::string& user, const std::string& pass, const SaveSyncEntry& entry, std::string& error);
    bool DownloadSaveToConsole(const std::string& shopUrl, const std::string& user, const std::string& pass, const SaveSyncEntry& entry, std::string& error);
}
