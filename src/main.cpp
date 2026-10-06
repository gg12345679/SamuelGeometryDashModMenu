#include <Geode/Geode.hpp>
#include <Geode/modify/OptionsLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/ui/Popup.hpp>

#include <fmt/format.h>
#include <algorithm>

using namespace geode::prelude;

static bool g_menuPausedGame = false;

class SamuelPopup;

static SamuelPopup* g_samuelPopup = nullptr;


// ======================================================
// SAVED OPTIONS
// ======================================================

static bool getNoclip() {
    return Mod::get()->getSavedValue<bool>(
        "noclip",
        false
    );
}

static bool getHitboxes() {
    return Mod::get()->getSavedValue<bool>(
        "hitboxes",
        false
    );
}

static bool getAutoPractice() {
    return Mod::get()->getSavedValue<bool>(
        "auto-practice",
        false
    );
}

static float getSpeed() {
    return Mod::get()->getSavedValue<float>(
        "speed",
        1.0f
    );
}


// ======================================================
// SPEED
// ======================================================

static void applySpeed() {

    if (g_menuPausedGame) {
        return;
    }

    float speed = std::clamp(
        getSpeed(),
        0.25f,
        4.0f
    );

    CCDirector::get()
        ->getScheduler()
        ->setTimeScale(speed);
}


// ======================================================
// MOD MENU
// ======================================================

class SamuelPopup : public geode::Popup {

protected:

    ButtonSprite* m_noclipSprite = nullptr;
    ButtonSprite* m_hitboxSprite = nullptr;
    ButtonSprite* m_practiceSprite = nullptr;
    CCLabelBMFont* m_speedLabel = nullptr;


    bool initSamuel() {

        if (!geode::Popup::init(
            390.f,
            280.f
        )) {
            return false;
        }

        this->setTitle(
            "Samuel Mod Menu"
        );


        auto category =
            CCLabelBMFont::create(
                "GAMEPLAY",
                "goldFont.fnt"
            );

        category->setScale(
            0.55f
        );

        m_mainLayer->addChildAtPosition(
            category,
            Anchor::Top,
            ccp(
                0.f,
                -45.f
            )
        );


        // NOCLIP

        m_noclipSprite =
            ButtonSprite::create(
                "Noclip: OFF",
                130,
                0,
                0.65f,
                true,
                "goldFont.fnt",
                "GJ_button_01.png",
                0.f
            );

        auto noclipButton =
            CCMenuItemSpriteExtra::create(
                m_noclipSprite,
                this,
                menu_selector(
                    SamuelPopup::onNoclip
                )
            );

        m_buttonMenu->addChildAtPosition(
            noclipButton,
            Anchor::Center,
            ccp(
                -90.f,
                45.f
            )
        );


        // HITBOXES

        m_hitboxSprite =
            ButtonSprite::create(
                "Hitboxes: OFF",
                130,
                0,
                0.65f,
                true,
                "goldFont.fnt",
                "GJ_button_01.png",
                0.f
            );

        auto hitboxButton =
            CCMenuItemSpriteExtra::create(
                m_hitboxSprite,
                this,
                menu_selector(
                    SamuelPopup::onHitboxes
                )
            );

        m_buttonMenu->addChildAtPosition(
            hitboxButton,
            Anchor::Center,
            ccp(
                90.f,
                45.f
            )
        );


        // AUTO PRACTICE

        m_practiceSprite =
            ButtonSprite::create(
                "Auto Practice: OFF",
                130,
                0,
                0.55f,
                true,
                "goldFont.fnt",
                "GJ_button_01.png",
                0.f
            );

        auto practiceButton =
            CCMenuItemSpriteExtra::create(
                m_practiceSprite,
                this,
                menu_selector(
                    SamuelPopup::onPractice
                )
            );

        m_buttonMenu->addChildAtPosition(
            practiceButton,
            Anchor::Center,
            ccp(
                -90.f,
                -15.f
            )
        );


        // RESTART

        auto restartSprite =
            ButtonSprite::create(
                "Restart Level",
                130,
                0,
                0.65f,
                true,
                "goldFont.fnt",
                "GJ_button_01.png",
                0.f
            );

        auto restartButton =
            CCMenuItemSpriteExtra::create(
                restartSprite,
                this,
                menu_selector(
                    SamuelPopup::onRestart
                )
            );

        m_buttonMenu->addChildAtPosition(
            restartButton,
            Anchor::Center,
            ccp(
                90.f,
                -15.f
            )
        );


        // SPEED -

        auto slowerSprite =
            ButtonSprite::create(
                "-",
                45,
                0,
                0.8f,
                true,
                "bigFont.fnt",
                "GJ_button_01.png",
                0.f
            );

        auto slowerButton =
            CCMenuItemSpriteExtra::create(
                slowerSprite,
                this,
                menu_selector(
                    SamuelPopup::onSlower
                )
            );

        m_buttonMenu->addChildAtPosition(
            slowerButton,
            Anchor::Bottom,
            ccp(
                -90.f,
                45.f
            )
        );


        // SPEED TEXT

        m_speedLabel =
            CCLabelBMFont::create(
                "",
                "bigFont.fnt"
            );

        m_speedLabel->setScale(
            0.55f
        );

        m_mainLayer->addChildAtPosition(
            m_speedLabel,
            Anchor::Bottom,
            ccp(
                0.f,
                47.f
            )
        );


        // SPEED +

        auto fasterSprite =
            ButtonSprite::create(
                "+",
                45,
                0,
                0.8f,
                true,
                "bigFont.fnt",
                "GJ_button_01.png",
                0.f
            );

        auto fasterButton =
            CCMenuItemSpriteExtra::create(
                fasterSprite,
                this,
                menu_selector(
                    SamuelPopup::onFaster
                )
            );

        m_buttonMenu->addChildAtPosition(
            fasterButton,
            Anchor::Bottom,
            ccp(
                90.f,
                45.f
            )
        );


        // RESET SPEED

        auto resetSprite =
            ButtonSprite::create(
                "Reset 1x",
                100,
                0,
                0.6f,
                true,
                "goldFont.fnt",
                "GJ_button_01.png",
                0.f
            );

        auto resetButton =
            CCMenuItemSpriteExtra::create(
                resetSprite,
                this,
                menu_selector(
                    SamuelPopup::onNormalSpeed
                )
            );

        m_buttonMenu->addChildAtPosition(
            resetButton,
            Anchor::Bottom,
            ccp(
                0.f,
                16.f
            )
        );


        refresh();

        return true;
    }


    void refresh() {

        if (m_noclipSprite) {

            m_noclipSprite->setString(
                getNoclip()
                    ? "Noclip: ON"
                    : "Noclip: OFF"
            );
        }


        if (m_hitboxSprite) {

            m_hitboxSprite->setString(
                getHitboxes()
                    ? "Hitboxes: ON"
                    : "Hitboxes: OFF"
            );
        }


        if (m_practiceSprite) {

            m_practiceSprite->setString(
                getAutoPractice()
                    ? "Auto Practice: ON"
                    : "Auto Practice: OFF"
            );
        }


        if (m_speedLabel) {

            auto text =
                fmt::format(
                    "Speed {:.2f}x",
                    getSpeed()
                );

            m_speedLabel->setString(
                text.c_str()
            );
        }
    }


    void onNoclip(
        CCObject*
    ) {

        Mod::get()->setSavedValue(
            "noclip",
            !getNoclip()
        );

        refresh();
    }


    void onHitboxes(
        CCObject*
    ) {

        bool enabled =
            !getHitboxes();

        Mod::get()->setSavedValue(
            "hitboxes",
            enabled
        );


        if (
            auto play =
                PlayLayer::get()
        ) {

            play->toggleDebugDraw();

            if (
                play->m_debugDrawNode
            ) {

                play
                    ->m_debugDrawNode
                    ->setVisible(
                        enabled
                    );
            }
        }


        refresh();
    }


    void onPractice(
        CCObject*
    ) {

        bool enabled =
            !getAutoPractice();

        Mod::get()->setSavedValue(
            "auto-practice",
            enabled
        );


        if (
            auto play =
                PlayLayer::get()
        ) {

            play->togglePracticeMode(
                enabled
            );
        }


        refresh();
    }


    void onRestart(
        CCObject*
    ) {

        if (
            auto play =
                PlayLayer::get()
        ) {

            this->onClose(
                nullptr
            );

            play->resetLevel();
        }

        else {

            FLAlertLayer::create(
                "Samuel Mod Menu",
                "Start a level first.",
                "OK"
            )->show();
        }
    }


    void onSlower(
        CCObject*
    ) {

        float speed =
            std::clamp(
                getSpeed() - 0.25f,
                0.25f,
                4.0f
            );

        Mod::get()->setSavedValue(
            "speed",
            speed
        );

        applySpeed();

        refresh();
    }


    void onFaster(
        CCObject*
    ) {

        float speed =
            std::clamp(
                getSpeed() + 0.25f,
                0.25f,
                4.0f
            );

        Mod::get()->setSavedValue(
            "speed",
            speed
        );

        applySpeed();

        refresh();
    }


    void onNormalSpeed(
        CCObject*
    ) {

        Mod::get()->setSavedValue(
            "speed",
            1.0f
        );

        applySpeed();

        refresh();
    }


    // CLOSE MENU AND UNFREEZE LEVEL

    void onClose(
        CCObject* sender
    ) override {

        g_samuelPopup =
            nullptr;


        if (
            g_menuPausedGame
        ) {

            if (
                auto play =
                    PlayLayer::get()
            ) {

                play->m_isPaused =
                    false;

                play
                    ->resumeSchedulerAndActions();
            }


            g_menuPausedGame =
                false;

            applySpeed();
        }


        geode::Popup::onClose(
            sender
        );
    }


public:

    static SamuelPopup* create() {

        auto ret =
            new SamuelPopup();


        if (
            ret &&
            ret->initSamuel()
        ) {

            ret->autorelease();

            return ret;
        }


        delete ret;

        return nullptr;
    }


    void closeMenu() {

        this->onClose(
            nullptr
        );
    }
};


// ======================================================
// OPEN / CLOSE MENU
// ======================================================

static void toggleSamuelMenu() {

    // M PRESSED WHILE MENU IS OPEN

    if (
        g_samuelPopup &&
        g_samuelPopup->getParent()
    ) {

        g_samuelPopup
            ->closeMenu();

        return;
    }


    auto popup =
        SamuelPopup::create();


    if (!popup) {

        return;
    }


    // FREEZE GAME WITHOUT ESC MENU

    if (
        auto play =
            PlayLayer::get()
    ) {

        if (
            !play->m_isPaused
        ) {

            play->m_isPaused =
                true;

            play
                ->pauseSchedulerAndActions();

            g_menuPausedGame =
                true;
        }
    }


    g_samuelPopup =
        popup;


    popup->show();
}


// ======================================================
// M HOTKEY
// ======================================================

class $modify(
    SamuelKeyboardHook,
    cocos2d::CCKeyboardDispatcher
) {

    bool dispatchKeyboardMSG(
        cocos2d::enumKeyCodes key,
        bool down,
        bool repeat,
        double timestamp
    ) {

        if (
            down &&
            !repeat &&
            key == cocos2d::KEY_M
        ) {

            toggleSamuelMenu();

            return true;
        }


        return
            cocos2d::CCKeyboardDispatcher::
            dispatchKeyboardMSG(
                key,
                down,
                repeat,
                timestamp
            );
    }
};


// ======================================================
// SETTINGS BUTTON
// ======================================================

class $modify(
    SamuelOptionsLayer,
    OptionsLayer
) {

    void customSetup() {

        OptionsLayer::customSetup();


        auto menu =
            CCMenu::create();


        menu->setPosition({
            0.f,
            0.f
        });


        menu->setID(
            "samuel-mod-menu"
        );


        auto buttonSprite =
            ButtonSprite::create(
                "SAMUEL MODS",
                105,
                0,
                0.7f,
                true,
                "goldFont.fnt",
                "GJ_button_01.png",
                0.f
            );


        auto button =
            CCMenuItemSpriteExtra::create(
                buttonSprite,
                this,
                menu_selector(
                    SamuelOptionsLayer::
                    onSamuelMods
                )
            );


        button->setScale(
            0.65f
        );


        auto size =
            CCDirector::get()
                ->getWinSize();


        button->setPosition({

            size.width / 2.f + 150.f,

            size.height / 2.f - 112.f
        });


        menu->addChild(
            button
        );


        this
            ->m_mainLayer
            ->addChild(
                menu,
                100
            );
    }


    void onSamuelMods(
        CCObject*
    ) {

        toggleSamuelMenu();
    }
};


// ======================================================
// GAMEPLAY MODS
// ======================================================

class $modify(
    SamuelPlayLayer,
    PlayLayer
) {

    bool init(
        GJGameLevel* level,
        bool useReplay,
        bool dontCreateObjects
    ) {

        if (
            !PlayLayer::init(
                level,
                useReplay,
                dontCreateObjects
            )
        ) {

            return false;
        }


        applySpeed();


        if (
            getAutoPractice()
        ) {

            this->togglePracticeMode(
                true
            );
        }


        if (
            getHitboxes()
        ) {

            this->toggleDebugDraw();


            if (
                this->m_debugDrawNode
            ) {

                this
                    ->m_debugDrawNode
                    ->setVisible(
                        true
                    );
            }
        }


        return true;
    }


    void destroyPlayer(
        PlayerObject* player,
        GameObject* object
    ) {

        if (
            getNoclip() &&
            !m_levelEndAnimationStarted
        ) {

            return;
        }


        PlayLayer::destroyPlayer(
            player,
            object
        );
    }
};
