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


// =====================================================
// SETTINGS
// =====================================================

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

static bool getAutoRetry() {
    return Mod::get()->getSavedValue<bool>(
        "auto-retry",
        false
    );
}

static bool getTrailEnabled() {
    return Mod::get()->getSavedValue<bool>(
        "trail-enabled",
        true
    );
}

static float getSpeed() {
    return Mod::get()->getSavedValue<float>(
        "speed",
        1.0f
    );
}


// =====================================================
// SPEED
// =====================================================

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


// =====================================================
// TRAILS
// =====================================================

static void setPlayerTrail(
    PlayerObject* player,
    bool visible
) {

    if (!player) {
        return;
    }

    if (player->m_regularTrail) {

        player
            ->m_regularTrail
            ->setVisible(visible);
    }

    if (player->m_waveTrail) {

        player
            ->m_waveTrail
            ->setVisible(visible);
    }
}

static void applyTrails(
    PlayLayer* play
) {

    if (!play) {
        return;
    }

    bool visible =
        getTrailEnabled();

    setPlayerTrail(
        play->m_player1,
        visible
    );

    setPlayerTrail(
        play->m_player2,
        visible
    );
}


// =====================================================
// SAMUEL POPUP
// =====================================================

class SamuelPopup : public geode::Popup {

protected:

    CCMenu* m_gameplayMenu = nullptr;

    CCMenu* m_toolsMenu = nullptr;

    ButtonSprite* m_noclipSprite = nullptr;

    ButtonSprite* m_hitboxSprite = nullptr;

    ButtonSprite* m_practiceSprite = nullptr;

    ButtonSprite* m_retrySprite = nullptr;

    ButtonSprite* m_trailSprite = nullptr;

    CCLabelBMFont* m_speedLabel = nullptr;


    // =================================================
    // BUTTON MAKER
    // =================================================

    CCMenuItemSpriteExtra* addButton(
        CCMenu* menu,
        char const* text,
        float width,
        float fontScale,
        cocos2d::SEL_MenuHandler callback,
        cocos2d::CCPoint position,
        ButtonSprite** saveSprite = nullptr
    ) {

        auto sprite =
            ButtonSprite::create(
                text,
                width,
                0,
                fontScale,
                true,
                "goldFont.fnt",
                "GJ_button_01.png",
                0.f
            );

        if (saveSprite) {

            *saveSprite =
                sprite;
        }

        auto button =
            CCMenuItemSpriteExtra::create(
                sprite,
                this,
                callback
            );

        button->setPosition(
            position
        );

        menu->addChild(
            button
        );

        return button;
    }


    // =================================================
    // CREATE MENU
    // =================================================

    bool initSamuel() {

        if (
            !geode::Popup::init(
                430.f,
                305.f
            )
        ) {

            return false;
        }

        this->setTitle(
            "Samuel Mod Menu"
        );


        auto size =
            m_mainLayer
                ->getContentSize();

        float cx =
            size.width / 2.f;


        // =================================================
        // TAB MENU
        // =================================================

        auto tabMenu =
            CCMenu::create();

        tabMenu->setPosition({
            0.f,
            0.f
        });

        m_mainLayer->addChild(
            tabMenu,
            20
        );


        auto gameplayTab =
            addButton(
                tabMenu,
                "GAMEPLAY",
                120.f,
                0.6f,
                menu_selector(
                    SamuelPopup::
                    onGameplayTab
                ),
                {
                    cx - 70.f,
                    size.height - 52.f
                }
            );

        gameplayTab->setScale(
            0.85f
        );


        auto toolsTab =
            addButton(
                tabMenu,
                "TOOLS",
                120.f,
                0.6f,
                menu_selector(
                    SamuelPopup::
                    onToolsTab
                ),
                {
                    cx + 70.f,
                    size.height - 52.f
                }
            );

        toolsTab->setScale(
            0.85f
        );


        // =================================================
        // GAMEPLAY TAB
        // =================================================

        m_gameplayMenu =
            CCMenu::create();

        m_gameplayMenu->setPosition({
            0.f,
            0.f
        });

        m_mainLayer->addChild(
            m_gameplayMenu,
            10
        );


        addButton(
            m_gameplayMenu,
            "Noclip: OFF",
            145.f,
            0.58f,
            menu_selector(
                SamuelPopup::onNoclip
            ),
            {
                cx - 92.f,
                188.f
            },
            &m_noclipSprite
        );


        addButton(
            m_gameplayMenu,
            "Hitboxes: OFF",
            145.f,
            0.58f,
            menu_selector(
                SamuelPopup::onHitboxes
            ),
            {
                cx + 92.f,
                188.f
            },
            &m_hitboxSprite
        );


        addButton(
            m_gameplayMenu,
            "Auto Practice: OFF",
            145.f,
            0.52f,
            menu_selector(
                SamuelPopup::onPractice
            ),
            {
                cx - 92.f,
                132.f
            },
            &m_practiceSprite
        );


        addButton(
            m_gameplayMenu,
            "Auto Retry: OFF",
            145.f,
            0.55f,
            menu_selector(
                SamuelPopup::onAutoRetry
            ),
            {
                cx + 92.f,
                132.f
            },
            &m_retrySprite
        );


        addButton(
            m_gameplayMenu,
            "Trail: ON",
            145.f,
            0.58f,
            menu_selector(
                SamuelPopup::onTrail
            ),
            {
                cx - 92.f,
                76.f
            },
            &m_trailSprite
        );


        addButton(
            m_gameplayMenu,
            "Restart Level",
            145.f,
            0.58f,
            menu_selector(
                SamuelPopup::onRestart
            ),
            {
                cx + 92.f,
                76.f
            }
        );


        // =================================================
        // TOOLS TAB
        // =================================================

        m_toolsMenu =
            CCMenu::create();

        m_toolsMenu->setPosition({
            0.f,
            0.f
        });

        m_mainLayer->addChild(
            m_toolsMenu,
            10
        );


        addButton(
            m_toolsMenu,
            "Place Checkpoint",
            150.f,
            0.54f,
            menu_selector(
                SamuelPopup::
                onPlaceCheckpoint
            ),
            {
                cx - 92.f,
                184.f
            }
        );


        addButton(
            m_toolsMenu,
            "Remove Checkpoint",
            150.f,
            0.50f,
            menu_selector(
                SamuelPopup::
                onRemoveCheckpoint
            ),
            {
                cx + 92.f,
                184.f
            }
        );


        auto speedTitle =
            CCLabelBMFont::create(
                "SPEED PRESETS",
                "goldFont.fnt"
            );

        speedTitle->setScale(
            0.48f
        );

        speedTitle->setPosition({
            cx,
            139.f
        });

        m_toolsMenu->addChild(
            speedTitle
        );


        // =================================================
        // SPEED PRESETS
        // =================================================

        addButton(
            m_toolsMenu,
            "0.5x",
            75.f,
            0.6f,
            menu_selector(
                SamuelPopup::onSpeed05
            ),
            {
                cx - 135.f,
                99.f
            }
        );


        addButton(
            m_toolsMenu,
            "1x",
            75.f,
            0.6f,
            menu_selector(
                SamuelPopup::onSpeed1
            ),
            {
                cx - 45.f,
                99.f
            }
        );


        addButton(
            m_toolsMenu,
            "2x",
            75.f,
            0.6f,
            menu_selector(
                SamuelPopup::onSpeed2
            ),
            {
                cx + 45.f,
                99.f
            }
        );


        addButton(
            m_toolsMenu,
            "4x",
            75.f,
            0.6f,
            menu_selector(
                SamuelPopup::onSpeed4
            ),
            {
                cx + 135.f,
                99.f
            }
        );


        m_speedLabel =
            CCLabelBMFont::create(
                "",
                "bigFont.fnt"
            );

        m_speedLabel->setScale(
            0.52f
        );

        m_speedLabel->setPosition({
            cx,
            53.f
        });

        m_toolsMenu->addChild(
            m_speedLabel
        );


        showGameplay();

        refresh();

        return true;
    }


    // =================================================
    // TABS
    // =================================================

    void showGameplay() {

        if (m_gameplayMenu) {

            m_gameplayMenu
                ->setVisible(true);
        }

        if (m_toolsMenu) {

            m_toolsMenu
                ->setVisible(false);
        }
    }


    void showTools() {

        if (m_gameplayMenu) {

            m_gameplayMenu
                ->setVisible(false);
        }

        if (m_toolsMenu) {

            m_toolsMenu
                ->setVisible(true);
        }
    }


    void onGameplayTab(
        CCObject*
    ) {

        showGameplay();
    }


    void onToolsTab(
        CCObject*
    ) {

        showTools();
    }


    // =================================================
    // REFRESH TEXT
    // =================================================

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


        if (m_retrySprite) {

            m_retrySprite->setString(
                getAutoRetry()
                    ? "Auto Retry: ON"
                    : "Auto Retry: OFF"
            );
        }


        if (m_trailSprite) {

            m_trailSprite->setString(
                getTrailEnabled()
                    ? "Trail: ON"
                    : "Trail: OFF"
            );
        }


        if (m_speedLabel) {

            auto text =
                fmt::format(
                    "Current Speed {:.2f}x",
                    getSpeed()
                );

            m_speedLabel->setString(
                text.c_str()
            );
        }
    }


    // =================================================
    // NOCLIP
    // =================================================

    void onNoclip(
        CCObject*
    ) {

        Mod::get()->setSavedValue(
            "noclip",
            !getNoclip()
        );

        refresh();
    }


    // =================================================
    // HITBOXES
    // =================================================

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


    // =================================================
    // PRACTICE
    // =================================================

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


    // =================================================
    // AUTO RETRY
    // =================================================

    void onAutoRetry(
        CCObject*
    ) {

        Mod::get()->setSavedValue(
            "auto-retry",
            !getAutoRetry()
        );

        refresh();
    }


    // =================================================
    // TRAIL
    // =================================================

    void onTrail(
        CCObject*
    ) {

        Mod::get()->setSavedValue(
            "trail-enabled",
            !getTrailEnabled()
        );

        applyTrails(
            PlayLayer::get()
        );

        refresh();
    }


    // =================================================
    // RESTART
    // =================================================

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


    // =================================================
    // PLACE CHECKPOINT
    // =================================================

    void onPlaceCheckpoint(
        CCObject*
    ) {

        if (
            auto play =
                PlayLayer::get()
        ) {

            this->onClose(
                nullptr
            );

            play->togglePracticeMode(
                true
            );

            play->createCheckpoint();
        }

        else {

            FLAlertLayer::create(
                "Samuel Mod Menu",
                "Start a level first.",
                "OK"
            )->show();
        }
    }


    // =================================================
    // REMOVE CHECKPOINT
    // =================================================

    void onRemoveCheckpoint(
        CCObject*
    ) {

        if (
            auto play =
                PlayLayer::get()
        ) {

            this->onClose(
                nullptr
            );

            play->removeCheckpoint(
                false
            );
        }

        else {

            FLAlertLayer::create(
                "Samuel Mod Menu",
                "Start a level first.",
                "OK"
            )->show();
        }
    }


    // =================================================
    // SPEED PRESET HELPER
    // =================================================

    void setSpeedPreset(
        float speed
    ) {

        Mod::get()->setSavedValue(
            "speed",
            speed
        );

        applySpeed();

        refresh();
    }


    void onSpeed05(
        CCObject*
    ) {

        setSpeedPreset(
            0.5f
        );
    }


    void onSpeed1(
        CCObject*
    ) {

        setSpeedPreset(
            1.0f
        );
    }


    void onSpeed2(
        CCObject*
    ) {

        setSpeedPreset(
            2.0f
        );
    }


    void onSpeed4(
        CCObject*
    ) {

        setSpeedPreset(
            4.0f
        );
    }


    // =================================================
    // CLOSE MENU
    // =================================================

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


// =====================================================
// OPEN CLOSE MENU
// =====================================================

static void toggleSamuelMenu() {

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


    // FREEZE LEVEL WITHOUT ESC MENU

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


// =====================================================
// M HOTKEY
// =====================================================

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


// =====================================================
// SETTINGS BUTTON
// =====================================================

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


// =====================================================
// PLAY LAYER
// =====================================================

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


        applyTrails(
            this
        );


        return true;
    }


    void destroyPlayer(
        PlayerObject* player,
        GameObject* object
    ) {

        // NOCLIP

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


        // FAST AUTO RETRY

        if (
            getAutoRetry() &&
            !m_levelEndAnimationStarted
        ) {

            this->delayedResetLevel();
        }
    }
};
