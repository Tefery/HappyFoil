#include <algorithm>
#include <filesystem>
#include <switch.h>
#include "ui/MainApplication.hpp"
#include "ui/shopInstPage.hpp"
#include "util/config.hpp"
#include "util/lang.hpp"
#include "util/util.hpp"

#define COLOR(hex) pu::ui::Color::FromHex(hex)

namespace inst::ui {
    extern MainApplication *mainApp;

    shopInstPage::shopInstPage() : Layout::Layout() {
        this->SetBackgroundColor(COLOR("#670000FF"));
        if (std::filesystem::exists(inst::config::appDir + "/background.png")) this->SetBackgroundImage(inst::config::appDir + "/background.png");
        else this->SetBackgroundImage("romfs:/images/background.jpg");
        this->topRect = Rectangle::New(0, 0, 1280, 94, COLOR("#170909FF"));
        this->infoRect = Rectangle::New(0, 95, 1280, 60, COLOR("#17090980"));
        this->botRect = Rectangle::New(0, 660, 1280, 60, COLOR("#17090980"));
        if (inst::config::gayMode) {
            this->titleImage = Image::New(-113, 0, "romfs:/images/logo.png");
            this->appVersionText = TextBlock::New(367, 49, "v" + inst::config::appVersion, 22);
        }
        else {
            this->titleImage = Image::New(0, 0, "romfs:/images/logo.png");
            this->appVersionText = TextBlock::New(480, 49, "v" + inst::config::appVersion, 22);
        }
        this->appVersionText->SetColor(COLOR("#FFFFFFFF"));
        this->pageInfoText = TextBlock::New(10, 109, "", 30);
        this->pageInfoText->SetColor(COLOR("#FFFFFFFF"));
        this->butText = TextBlock::New(10, 678, "", 24);
        this->butText->SetColor(COLOR("#FFFFFFFF"));
        this->menu = pu::ui::elm::Menu::New(0, 156, 1280, COLOR("#FFFFFF00"), 84, (506 / 84));
        this->menu->SetOnFocusColor(COLOR("#00000033"));
        this->menu->SetScrollbarColor(COLOR("#17090980"));
        this->infoImage = Image::New(453, 292, "romfs:/images/icons/lan-connection-waiting.png");
        this->Add(this->topRect);
        this->Add(this->infoRect);
        this->Add(this->botRect);
        this->Add(this->titleImage);
        this->Add(this->appVersionText);
        this->Add(this->butText);
        this->Add(this->pageInfoText);
        this->Add(this->menu);
        this->Add(this->infoImage);
    }

    void shopInstPage::drawMenuItems(bool clearItems) {
        if (clearItems) this->selectedItems.clear();
        this->menu->ClearItems();
        for (const auto& item : this->shopItems) {
            std::string itm = inst::util::shortenString(item.name, 56, true);
            auto entry = pu::ui::elm::MenuItem::New(itm);
            entry->SetColor(COLOR("#FFFFFFFF"));
            entry->SetIcon("romfs:/images/icons/checkbox-blank-outline.png");
            for (const auto& selected : this->selectedItems) {
                if (selected.url == item.url) {
                    entry->SetIcon("romfs:/images/icons/check-box-outline.png");
                    break;
                }
            }
            this->menu->AddItem(entry);
        }
    }

    void shopInstPage::selectTitle(int selectedIndex) {
        const auto& item = this->shopItems[selectedIndex];
        auto selected = std::find_if(this->selectedItems.begin(), this->selectedItems.end(), [&](const auto& entry) {
            return entry.url == item.url;
        });
        if (selected != this->selectedItems.end())
            this->selectedItems.erase(selected);
        else
            this->selectedItems.push_back(item);
        this->drawMenuItems(false);
    }

    void shopInstPage::startShop() {
        this->butText->SetText("inst.shop.buttons_loading"_lang);
        this->menu->SetVisible(false);
        this->menu->ClearItems();
        this->infoImage->SetVisible(true);
        this->pageInfoText->SetText("inst.shop.loading"_lang);
        mainApp->LoadLayout(mainApp->shopinstPage);

        std::string shopUrl = inst::config::shopUrl;
        if (shopUrl.empty()) {
            shopUrl = inst::util::softwareKeyboard("options.shop.url_hint"_lang, "http://", 200);
            if (shopUrl.empty()) {
                mainApp->LoadLayout(mainApp->mainPage);
                return;
            }
            inst::config::shopUrl = shopUrl;
            inst::config::setConfig();
        }

        std::string error;
        this->shopItems = shopInstStuff::FetchShop(shopUrl, inst::config::shopUser, inst::config::shopPass, error);
        if (!error.empty()) {
            mainApp->CreateShowDialog("inst.shop.failed"_lang, error, {"common.ok"_lang}, true);
            mainApp->LoadLayout(mainApp->mainPage);
            return;
        }
        if (this->shopItems.empty()) {
            mainApp->CreateShowDialog("inst.shop.empty"_lang, "", {"common.ok"_lang}, true);
            mainApp->LoadLayout(mainApp->mainPage);
            return;
        }

        this->pageInfoText->SetText("inst.shop.top_info"_lang);
        this->butText->SetText("inst.shop.buttons"_lang);
        this->drawMenuItems(true);
        this->menu->SetSelectedIndex(0);
        this->infoImage->SetVisible(false);
        this->menu->SetVisible(true);
    }

    void shopInstPage::startInstall() {
        int dialogResult = -1;
        if (this->selectedItems.size() == 1) {
            std::string name = inst::util::shortenString(this->selectedItems[0].name, 32, true);
            dialogResult = mainApp->CreateShowDialog("inst.target.desc0"_lang + name + "inst.target.desc1"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        } else {
            dialogResult = mainApp->CreateShowDialog("inst.target.desc00"_lang + std::to_string(this->selectedItems.size()) + "inst.target.desc01"_lang, "common.cancel_desc"_lang, {"inst.target.opt0"_lang, "inst.target.opt1"_lang}, false);
        }
        if (dialogResult == -1)
            return;

        shopInstStuff::installTitleShop(this->selectedItems, dialogResult, "inst.shop.source_string"_lang);
    }

    void shopInstPage::onInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos) {
        if (Down & HidNpadButton_B) {
            mainApp->LoadLayout(mainApp->mainPage);
        }
        if ((Down & HidNpadButton_A) || (Up & TouchPseudoKey)) {
            this->selectTitle(this->menu->GetSelectedIndex());
            if (this->menu->GetItems().size() == 1 && this->selectedItems.size() == 1) {
                this->startInstall();
            }
        }
        if (Down & HidNpadButton_Y) {
            if (this->selectedItems.size() == this->menu->GetItems().size()) {
                this->drawMenuItems(true);
            } else {
                for (long unsigned int i = 0; i < this->menu->GetItems().size(); i++) {
                    if (this->menu->GetItems()[i]->GetIcon() == "romfs:/images/icons/check-box-outline.png") continue;
                    this->selectTitle(i);
                }
                this->drawMenuItems(false);
            }
        }
        if (Down & HidNpadButton_X) {
            this->startShop();
        }
        if (Down & HidNpadButton_Plus) {
            if (this->selectedItems.empty()) {
                this->selectTitle(this->menu->GetSelectedIndex());
            }
            if (!this->selectedItems.empty()) this->startInstall();
        }
    }
}
