#pragma once

#include <pu/Plutonium>
#include "shopInstall.hpp"

using namespace pu::ui::elm;
namespace inst::ui {
    class shopInstPage : public pu::ui::Layout
    {
        public:
            shopInstPage();
            PU_SMART_CTOR(shopInstPage)
            void startShop();
            void startInstall();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos);
            TextBlock::Ref pageInfoText;
            Image::Ref titleImage;
            TextBlock::Ref appVersionText;
        private:
            std::vector<shopInstStuff::ShopItem> shopItems;
            std::vector<shopInstStuff::ShopItem> selectedItems;
            TextBlock::Ref butText;
            Rectangle::Ref topRect;
            Rectangle::Ref infoRect;
            Rectangle::Ref botRect;
            pu::ui::elm::Menu::Ref menu;
            Image::Ref infoImage;
            void drawMenuItems(bool clearItems);
            void selectTitle(int selectedIndex);
    };
}
