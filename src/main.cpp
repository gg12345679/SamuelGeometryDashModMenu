#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

class $modify(SamuelMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        auto button = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("SAMUEL MODS"),
            this,
            menu_selector(SamuelMenuLayer::onSamuelMods)
        );

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});

        auto size = CCDirector::get()->getWinSize();
        button->setPosition({90.f, size.height - 30.f});

        menu->addChild(button);
        this->addChild(menu, 100);

        return true;
    }

    void onSamuelMods(CCObject*) {
        FLAlertLayer::create(
            "Samuel Mod Menu",
            "Your custom mod menu is working!\n\nNext we can add Noclip, Speed Hack, Practice Tools and more.",
            "OK"
        )->show();
    }
};
