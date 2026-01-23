#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace shopInstStuff {
    struct ShopItem {
        std::string name;
        std::string url;
        std::uint64_t size;
    };

    std::vector<ShopItem> FetchShop(const std::string& shopUrl, const std::string& user, const std::string& pass, std::string& error);
    void installTitleShop(const std::vector<ShopItem>& items, int storage, const std::string& sourceLabel);
}
