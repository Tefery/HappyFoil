#include <algorithm>
#include <cctype>
#include <curl/curl.h>
#include <filesystem>
#include <sstream>
#include <thread>
#include "shopInstall.hpp"
#include "install/http_nsp.hpp"
#include "install/http_xci.hpp"
#include "install/install.hpp"
#include "install/install_nsp.hpp"
#include "install/install_xci.hpp"
#include "ui/MainApplication.hpp"
#include "ui/instPage.hpp"
#include "util/config.hpp"
#include "util/error.hpp"
#include "util/json.hpp"
#include "util/lang.hpp"
#include "util/network_util.hpp"
#include "util/util.hpp"

namespace inst::ui {
    extern MainApplication *mainApp;
}

namespace {
    size_t WriteToString(char* ptr, size_t size, size_t numItems, void* userdata)
    {
        auto out = reinterpret_cast<std::string*>(userdata);
        out->append(ptr, size * numItems);
        return size * numItems;
    }

    std::string NormalizeShopUrl(std::string url)
    {
        url.erase(0, url.find_first_not_of(" \t\r\n"));
        url.erase(url.find_last_not_of(" \t\r\n") + 1);
        if (url.empty())
            return url;
        if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0)
            url = "http://" + url;
        if (!url.empty() && url.back() == '/')
            url.pop_back();
        return url;
    }

    std::string DecodeUrlSegment(const std::string& value)
    {
        CURL* curl = curl_easy_init();
        if (!curl)
            return value;
        int outLength = 0;
        char* decoded = curl_easy_unescape(curl, value.c_str(), value.size(), &outLength);
        std::string result = decoded ? std::string(decoded, outLength) : value;
        if (decoded)
            curl_free(decoded);
        curl_easy_cleanup(curl);
        return result;
    }

    std::vector<std::string> BuildTinfoilHeaders()
    {
        return {
            "Theme: Awoo-Installer",
            "Uid: 0000000000000000",
            "Version: 0.0",
            "Revision: 0",
            "Language: en",
            "Hauth: 0",
            "Uauth: 0"
        };
    }

    bool IsXciExtension(const std::string& name)
    {
        std::string ext = std::filesystem::path(name).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".xci" || ext == ".xcz";
    }
}

namespace shopInstStuff {
    std::vector<ShopItem> FetchShop(const std::string& shopUrl, const std::string& user, const std::string& pass, std::string& error)
    {
        std::vector<ShopItem> items;
        error.clear();

        std::string baseUrl = NormalizeShopUrl(shopUrl);
        if (baseUrl.empty()) {
            error = "Shop URL is empty.";
            return items;
        }

        CURL* curl = curl_easy_init();
        if (!curl) {
            error = "Failed to initialize curl.";
            return items;
        }

        std::string body;
        curl_easy_setopt(curl, CURLOPT_URL, baseUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "tinfoil");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 15000L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);

        struct curl_slist* headerList = nullptr;
        const auto headers = BuildTinfoilHeaders();
        for (const auto& header : headers)
            headerList = curl_slist_append(headerList, header.c_str());
        if (headerList)
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);

        std::string authValue;
        if (!user.empty() || !pass.empty()) {
            authValue = user + ":" + pass;
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
            curl_easy_setopt(curl, CURLOPT_USERPWD, authValue.c_str());
        }

        CURLcode rc = curl_easy_perform(curl);
        if (headerList)
            curl_slist_free_all(headerList);
        curl_easy_cleanup(curl);

        if (rc != CURLE_OK) {
            error = curl_easy_strerror(rc);
            return items;
        }

        if (body.rfind("TINFOIL", 0) == 0) {
            error = "Encrypted shop responses are not supported. Disable Encrypt shop in Ownfoil settings.";
            return items;
        }

        try {
            nlohmann::json shop = nlohmann::json::parse(body);
            if (shop.contains("error")) {
                error = shop["error"].get<std::string>();
                return items;
            }
            if (!shop.contains("files") || !shop["files"].is_array()) {
                error = "Shop response missing file list.";
                return items;
            }

            for (const auto& entry : shop["files"]) {
                if (!entry.contains("url"))
                    continue;
                std::string url = entry["url"].get<std::string>();
                std::uint64_t size = 0;
                if (entry.contains("size") && entry["size"].is_number()) {
                    size = entry["size"].get<std::uint64_t>();
                }

                std::string fragment;
                std::string urlPath = url;
                auto hashPos = urlPath.find('#');
                if (hashPos != std::string::npos) {
                    fragment = urlPath.substr(hashPos + 1);
                    urlPath = urlPath.substr(0, hashPos);
                }

                std::string fullUrl;
                if (urlPath.rfind("http://", 0) == 0 || urlPath.rfind("https://", 0) == 0)
                    fullUrl = urlPath;
                else if (!urlPath.empty() && urlPath[0] == '/')
                    fullUrl = baseUrl + urlPath;
                else
                    fullUrl = baseUrl + "/" + urlPath;

                std::string name;
                if (!fragment.empty())
                    name = DecodeUrlSegment(fragment);
                else
                    name = inst::util::formatUrlString(fullUrl);

                if (!fullUrl.empty() && !name.empty())
                    items.push_back({name, fullUrl, size});
            }
        }
        catch (...) {
            error = "Invalid shop response.";
            return {};
        }

        std::sort(items.begin(), items.end(), [](const ShopItem& a, const ShopItem& b) {
            return inst::util::ignoreCaseCompare(a.name, b.name);
        });
        return items;
    }

    void installTitleShop(const std::vector<ShopItem>& items, int storage, const std::string& sourceLabel)
    {
        inst::util::initInstallServices();
        inst::ui::instPage::loadInstallScreen();
        bool nspInstalled = true;
        NcmStorageId destStorageId = storage ? NcmStorageId_BuiltInUser : NcmStorageId_SdCard;

        std::vector<std::string> names;
        names.reserve(items.size());
        for (const auto& item : items)
            names.push_back(inst::util::shortenString(item.name, 38, true));

        std::vector<int> previousClockValues;
        if (inst::config::overClock) {
            previousClockValues.push_back(inst::util::setClockSpeed(0, 1785000000)[0]);
            previousClockValues.push_back(inst::util::setClockSpeed(1, 76800000)[0]);
            previousClockValues.push_back(inst::util::setClockSpeed(2, 1600000000)[0]);
        }

        if (!inst::config::shopUser.empty() || !inst::config::shopPass.empty())
            tin::network::SetBasicAuth(inst::config::shopUser, inst::config::shopPass);
        else
            tin::network::ClearBasicAuth();

        std::string currentName;
        try {
            for (size_t i = 0; i < items.size(); i++) {
                LOG_DEBUG("%s %s\n", "Install request from", items[i].url.c_str());
                currentName = names[i];
                inst::ui::instPage::setTopInstInfoText("inst.info_page.top_info0"_lang + currentName + sourceLabel);
                std::unique_ptr<tin::install::Install> installTask;

                if (IsXciExtension(items[i].name)) {
                    auto httpXCI = std::make_shared<tin::install::xci::HTTPXCI>(items[i].url);
                    installTask = std::make_unique<tin::install::xci::XCIInstallTask>(destStorageId, inst::config::ignoreReqVers, httpXCI);
                } else {
                    auto httpNSP = std::make_shared<tin::install::nsp::HTTPNSP>(items[i].url);
                    installTask = std::make_unique<tin::install::nsp::NSPInstall>(destStorageId, inst::config::ignoreReqVers, httpNSP);
                }

                LOG_DEBUG("%s\n", "Preparing installation");
                inst::ui::instPage::setInstInfoText("inst.info_page.preparing"_lang);
                inst::ui::instPage::setInstBarPerc(0);
                installTask->Prepare();
                installTask->Begin();
            }
        }
        catch (std::exception& e) {
            LOG_DEBUG("Failed to install");
            LOG_DEBUG("%s", e.what());
            fprintf(stdout, "%s", e.what());
            std::string failedName = currentName.empty() ? names.front() : currentName;
            inst::ui::instPage::setInstInfoText("inst.info_page.failed"_lang + failedName);
            inst::ui::instPage::setInstBarPerc(0);
            std::string audioPath = "romfs:/audio/bark.wav";
            if (inst::config::gayMode) audioPath = "";
            if (std::filesystem::exists(inst::config::appDir + "/bark.wav")) audioPath = inst::config::appDir + "/bark.wav";
            std::thread audioThread(inst::util::playAudio, audioPath);
            inst::ui::mainApp->CreateShowDialog("inst.info_page.failed"_lang + failedName + "!", "inst.info_page.failed_desc"_lang + "\n\n" + (std::string)e.what(), {"common.ok"_lang}, true);
            audioThread.join();
            nspInstalled = false;
        }

        tin::network::ClearBasicAuth();

        if (previousClockValues.size() > 0) {
            inst::util::setClockSpeed(0, previousClockValues[0]);
            inst::util::setClockSpeed(1, previousClockValues[1]);
            inst::util::setClockSpeed(2, previousClockValues[2]);
        }

        if (nspInstalled) {
            inst::ui::instPage::setInstInfoText("inst.info_page.complete"_lang);
            inst::ui::instPage::setInstBarPerc(100);
            std::string audioPath = "romfs:/audio/awoo.wav";
            if (inst::config::gayMode) audioPath = "";
            if (std::filesystem::exists(inst::config::appDir + "/awoo.wav")) audioPath = inst::config::appDir + "/awoo.wav";
            std::thread audioThread(inst::util::playAudio, audioPath);
            if (items.size() > 1)
                inst::ui::mainApp->CreateShowDialog(std::to_string(items.size()) + "inst.info_page.desc0"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
            else
                inst::ui::mainApp->CreateShowDialog(names.front() + "inst.info_page.desc1"_lang, Language::GetRandomMsg(), {"common.ok"_lang}, true);
            audioThread.join();
        }

        LOG_DEBUG("Done");
        inst::ui::instPage::loadMainMenu();
        inst::util::deinitInstallServices();
    }
}
