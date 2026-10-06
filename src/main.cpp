#include <Geode/Geode.hpp>
#include <Geode/modify/OptionsLayer.hpp>

using namespace geode::prelude;

class $modify(SamuelOptionsLayer, OptionsLayer) {
    void customSetup() {
        OptionsLayer::customSetup();

        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        menu->setID("samuel-mod-menu");

        auto buttonSprite = ButtonSprite::create(
            "SAMUEL MODS",
            105,
            0,
            0.7f,
            true,
            "goldFont.fnt",
            "GJ_button_01.png",
            0.0f
        );

        auto button = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(SamuelOptionsLayer::onSamuelMods)
        );

        button->setScale(0.65f);

        auto size = CCDirector::get()->getWinSize();

        button->setPosition({
            size.width / 2.f + 150.f,
            size.height / 2.f - 112.f
        });

        menu->addChild(button);
        this->m_mainLayer->addChild(menu, 100);
    }

    void onSamuelMods(CCObject*) {
        FLAlertLayer::create(
            "Samuel Mod Menu",
            "Your Samuel Mod Menu is working!\n\nNoclip\nSpeed Hack\nPractice Tools\nand more coming.",
            "OK"
        )->show();
    }
};
