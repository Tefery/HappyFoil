#pragma once
#include <pu/Plutonium>

using namespace pu::ui::elm;
namespace inst::ui {
    class netInstPage : public pu::ui::Layout
    {
        public:
            netInstPage();
            PU_SMART_CTOR(netInstPage)
            void startInstall(bool urlMode);
            void startNetwork();
            void onInput(u64 Down, u64 Up, u64 Held, pu::ui::Touch Pos);
            TextBlock::Ref pageInfoText;
            Image::Ref titleImage;
            TextBlock::Ref appVersionText;
            TextBlock::Ref timeText;
            TextBlock::Ref sysLabelText;
            TextBlock::Ref sysFreeText;
            TextBlock::Ref sdLabelText;
            TextBlock::Ref sdFreeText;
            Rectangle::Ref sysBarBack;
            Rectangle::Ref sysBarFill;
            Rectangle::Ref sdBarBack;
            Rectangle::Ref sdBarFill;
            Rectangle::Ref netIndicator;
            Rectangle::Ref wifiBar1;
            Rectangle::Ref wifiBar2;
            Rectangle::Ref wifiBar3;
            Rectangle::Ref batteryOutline;
            Rectangle::Ref batteryFill;
            Rectangle::Ref batteryCap;
        private:
            std::vector<std::string> ourUrls;
            std::vector<std::string> selectedUrls;
            std::vector<std::string> alternativeNames;
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
