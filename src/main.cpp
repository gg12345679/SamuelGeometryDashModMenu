#include <Geode/Geode.hpp>
#include <Geode/modify/OptionsLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/ui/Popup.hpp>

#include <fmt/format.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <string>
#include <utility>
#include <vector>

using namespace geode::prelude;

static bool g_menuPausedGame = false;
static bool g_slowMoHeld = false;
static bool g_resetDeathsRequested = false;
static std::deque<double> g_clickTimes;

class SamuelPopup;
static SamuelPopup* g_samuelPopup = nullptr;

static double nowSeconds() {
    using Clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(
        Clock::now().time_since_epoch()
    ).count();
}

// =====================================================
// SAVED SETTINGS
// =====================================================

static bool getBool(char const* key, bool def = false) {
    return Mod::get()->getSavedValue<bool>(key, def);
}

static void setBool(char const* key, bool value) {
    Mod::get()->setSavedValue(key, value);
}

static float getFloat(char const* key, float def) {
    return Mod::get()->getSavedValue<float>(key, def);
}

static void setFloat(char const* key, float value) {
    Mod::get()->setSavedValue(key, value);
}

static int getInt(char const* key, int def) {
    return Mod::get()->getSavedValue<int>(key, def);
}

static void setInt(char const* key, int value) {
    Mod::get()->setSavedValue(key, value);
}

static bool getNoclip() { return getBool("noclip"); }
static bool getAutoPractice() { return getBool("auto-practice"); }
static bool getAutoRetry() { return getBool("auto-retry"); }
static bool getInstantRestart() { return getBool("instant-restart"); }
static bool getPracticeMusic() { return getBool("practice-music"); }

static bool getShowFPS() { return getBool("show-fps"); }
static bool getShowCPS() { return getBool("show-cps"); }
static bool getShowAttempts() { return getBool("show-attempts"); }
static bool getShowDeaths() { return getBool("show-deaths"); }
static bool getShowPercent() { return getBool("show-percent"); }

static bool getNoDeathFX() { return getBool("no-death-fx"); }
static bool getHitboxesAlways() {
    return getBool("hitboxes-always", getBool("hitboxes", false));
}
static bool getPlayerHitboxOnly() { return getBool("player-hitbox-only"); }
static bool getObjectHitboxOnly() { return getBool("object-hitbox-only"); }
static bool getRainbowTrail() { return getBool("rainbow-trail"); }
static bool getLongTrail() { return getBool("long-trail"); }
static bool getTrailEnabled() { return getBool("trail-enabled", true); }
static bool getHidePlayer() { return getBool("hide-player"); }

static bool getAutoCheckpoints() { return getBool("auto-checkpoints"); }
static bool getSlowMoHotkey() { return getBool("slowmo-hotkey"); }
static bool getSpeedHotkeys() { return getBool("speed-hotkeys"); }

static float getSpeed() {
    return std::clamp(getFloat("speed", 1.0f), 0.25f, 4.0f);
}

static float getCheckpointDelay() {
    return std::clamp(getFloat("checkpoint-delay", 1.0f), 0.5f, 5.0f);
}

static float getPlayerScaleValue() {
    return std::clamp(getFloat("player-scale", 1.0f), 0.5f, 2.0f);
}

static int getFPSLimit() {
    int fps = getInt("fps-limit", 240);
    static constexpr std::array<int, 5> allowed = {60, 120, 144, 240, 360};
    if (std::find(allowed.begin(), allowed.end(), fps) == allowed.end()) {
        fps = 240;
    }
    return fps;
}

// =====================================================
// APPLY HELPERS
// =====================================================

static void applySpeed() {
    if (g_menuPausedGame) return;

    float speed = getSpeed();
    if (getSlowMoHotkey() && g_slowMoHeld) {
        speed = 0.25f;
    }

    if (auto director = CCDirector::get()) {
        if (auto scheduler = director->getScheduler()) {
            scheduler->setTimeScale(speed);
        }
    }
}

static void applyFPSLimit() {
    int fps = getFPSLimit();
    if (fps < 1) fps = 60;

    if (auto app = CCApplication::sharedApplication()) {
        app->setAnimationInterval(1.0 / static_cast<double>(fps));
    }
}

static void setSpeed(float speed) {
    setFloat("speed", std::clamp(speed, 0.25f, 4.0f));
    applySpeed();
}

static void setPlayerTrailVisible(PlayerObject* player, bool visible) {
    if (!player) return;

    if (player->m_regularTrail) {
        player->m_regularTrail->setVisible(visible);
    }
    if (player->m_waveTrail) {
        player->m_waveTrail->setVisible(visible);
    }
}

static void applyTrailVisibility(PlayLayer* play) {
    if (!play) return;
    bool visible = getTrailEnabled();
    setPlayerTrailVisible(play->m_player1, visible);
    setPlayerTrailVisible(play->m_player2, visible);
}

static void teleportPlayer(PlayerObject* player, float amount) {
    if (!player) return;
    auto pos = player->getPosition();
    player->setPosition({pos.x + amount, pos.y});
}

static void flipPlayerGravity(PlayerObject* player) {
    if (!player) return;
    player->flipGravity(!player->m_isUpsideDown, false);
}

// =====================================================
// ACTION IDS
// =====================================================

enum SamuelAction {
    ACT_NOCLIP = 1,
    ACT_AUTO_PRACTICE,
    ACT_AUTO_RETRY,
    ACT_INSTANT_RESTART,
    ACT_PRACTICE_MUSIC,
    ACT_GRAVITY,
    ACT_TELEPORT,
    ACT_RESTART,

    ACT_FPS_LIMIT,
    ACT_SHOW_FPS,
    ACT_SHOW_CPS,
    ACT_SHOW_ATTEMPTS,
    ACT_SHOW_DEATHS,
    ACT_SHOW_PERCENT,
    ACT_RESET_DEATHS,

    ACT_NO_DEATH_FX,
    ACT_HITBOX_ALWAYS,
    ACT_PLAYER_HITBOX_ONLY,
    ACT_OBJECT_HITBOX_ONLY,
    ACT_RAINBOW_TRAIL,
    ACT_LONG_TRAIL,
    ACT_TRAIL,
    ACT_HIDE_PLAYER,
    ACT_PLAYER_SCALE,

    ACT_AUTO_CHECKPOINTS,
    ACT_CHECKPOINT_DELAY,
    ACT_REMOVE_ALL_CHECKPOINTS,
    ACT_SLOWMO_HOTKEY,
    ACT_SPEED_HOTKEYS,
    ACT_PLACE_CHECKPOINT,
    ACT_REMOVE_CHECKPOINT,
    ACT_SPEED_05,
    ACT_SPEED_1,
    ACT_SPEED_2,
    ACT_SPEED_4
};

enum SamuelTab {
    TAB_GAME = 0,
    TAB_HUD,
    TAB_VISUAL,
    TAB_TOOLS,
    TAB_INFO,
    TAB_FAVORITES,
    TAB_COUNT
};

static std::string onOff(bool value) {
    return value ? "ON" : "OFF";
}

static std::string actionText(int action) {
    switch (action) {
        case ACT_NOCLIP: return "Noclip: " + onOff(getNoclip());
        case ACT_AUTO_PRACTICE: return "Auto Practice: " + onOff(getAutoPractice());
        case ACT_AUTO_RETRY: return "Auto Retry: " + onOff(getAutoRetry());
        case ACT_INSTANT_RESTART: return "Instant Restart: " + onOff(getInstantRestart());
        case ACT_PRACTICE_MUSIC: return "Practice Music: " + onOff(getPracticeMusic());
        case ACT_GRAVITY: return "Flip Gravity";
        case ACT_TELEPORT: return "Teleport +100";
        case ACT_RESTART: return "Restart Level";

        case ACT_FPS_LIMIT: return fmt::format("FPS Limit: {}", getFPSLimit());
        case ACT_SHOW_FPS: return "Show FPS: " + onOff(getShowFPS());
        case ACT_SHOW_CPS: return "Show CPS: " + onOff(getShowCPS());
        case ACT_SHOW_ATTEMPTS: return "Attempts: " + onOff(getShowAttempts());
        case ACT_SHOW_DEATHS: return "Deaths: " + onOff(getShowDeaths());
        case ACT_SHOW_PERCENT: return "Percent: " + onOff(getShowPercent());
        case ACT_RESET_DEATHS: return "Reset Death Counter";

        case ACT_NO_DEATH_FX: return "No Death FX: " + onOff(getNoDeathFX());
        case ACT_HITBOX_ALWAYS: return "Hitboxes Always: " + onOff(getHitboxesAlways());
        case ACT_PLAYER_HITBOX_ONLY: return "Player Hitbox: " + onOff(getPlayerHitboxOnly());
        case ACT_OBJECT_HITBOX_ONLY: return "Object Hitboxes: " + onOff(getObjectHitboxOnly());
        case ACT_RAINBOW_TRAIL: return "Rainbow Trail: " + onOff(getRainbowTrail());
        case ACT_LONG_TRAIL: return "Long Trail: " + onOff(getLongTrail());
        case ACT_TRAIL: return "Trail: " + onOff(getTrailEnabled());
        case ACT_HIDE_PLAYER: return "Hide Player: " + onOff(getHidePlayer());
        case ACT_PLAYER_SCALE: return fmt::format("Player Scale: {:.2f}x", getPlayerScaleValue());

        case ACT_AUTO_CHECKPOINTS: return "Auto Checkpoints: " + onOff(getAutoCheckpoints());
        case ACT_CHECKPOINT_DELAY: return fmt::format("CP Delay: {:.1f}s", getCheckpointDelay());
        case ACT_REMOVE_ALL_CHECKPOINTS: return "Remove ALL Checkpoints";
        case ACT_SLOWMO_HOTKEY: return "Slow-Mo S: " + onOff(getSlowMoHotkey());
        case ACT_SPEED_HOTKEYS: return "Q/W/E/R Speeds: " + onOff(getSpeedHotkeys());
        case ACT_PLACE_CHECKPOINT: return "Place Checkpoint";
        case ACT_REMOVE_CHECKPOINT: return "Remove Checkpoint";
        case ACT_SPEED_05: return "0.5x";
        case ACT_SPEED_1: return "1x";
        case ACT_SPEED_2: return "2x";
        case ACT_SPEED_4: return "4x";
    }
    return "Unknown";
}

// =====================================================
// POPUP
// =====================================================

class SamuelPopup : public geode::Popup {
protected:
    std::array<CCMenu*, TAB_COUNT> m_pages{};
    std::vector<std::pair<int, ButtonSprite*>> m_actionSprites;
    CCLabelBMFont* m_infoLabel = nullptr;
    CCLabelBMFont* m_speedLabel = nullptr;

    CCMenuItemSpriteExtra* addAction(
        CCMenu* page,
        int action,
        CCPoint pos,
        float width = 190.f,
        float scale = 0.48f
    ) {
        auto text = actionText(action);
        auto sprite = ButtonSprite::create(
            text.c_str(),
            static_cast<int>(width),
            0,
            scale,
            true,
            "goldFont.fnt",
            "GJ_button_01.png",
            0.f
        );

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(SamuelPopup::onAction)
        );

        button->setTag(action);
        button->setPosition(pos);
        page->addChild(button);
        m_actionSprites.emplace_back(action, sprite);
        return button;
    }

    CCMenuItemSpriteExtra* addTab(
        CCMenu* tabMenu,
        int tab,
        char const* text,
        CCPoint pos
    ) {
        auto sprite = ButtonSprite::create(
            text,
            76,
            0,
            0.45f,
            true,
            "goldFont.fnt",
            "GJ_button_04.png",
            0.f
        );

        auto button = CCMenuItemSpriteExtra::create(
            sprite,
            this,
            menu_selector(SamuelPopup::onTab)
        );

        button->setTag(tab);
        button->setScale(0.78f);
        button->setPosition(pos);
        tabMenu->addChild(button);
        return button;
    }

    void showTab(int tab) {
        for (int i = 0; i < TAB_COUNT; ++i) {
            if (m_pages[i]) {
                m_pages[i]->setVisible(i == tab);
            }
        }
        if (tab == TAB_INFO) {
            updateInfo();
        }
        refresh();
    }

    void updateInfo() {
        if (!m_infoLabel) return;

        auto play = PlayLayer::get();
        if (!play) {
            m_infoLabel->setString(
                "Start a level to see live level info.\n\n"
                "M = Open / Close Samuel Mods\n"
                "S = Slow motion (when enabled)\n"
                "Q/W/E/R = 0.5x / 1x / 2x / 4x"
            );
            return;
        }

        int levelID = play->m_level ? play->m_level->m_levelID.value() : 0;
        unsigned int checkpoints =
            play->m_checkpointArray ? play->m_checkpointArray->count() : 0;

        auto text = fmt::format(
            "LEVEL INFO\n\n"
            "Level ID: {}\n"
            "Attempts: {}\n"
            "Jumps: {}\n"
            "Percent: {:.2f}%\n"
            "Checkpoints: {}\n"
            "Game Speed: {:.2f}x\n"
            "FPS Limit: {}\n"
            "Practice: {}",
            levelID,
            play->m_attempts,
            play->m_jumps,
            play->getCurrentPercent(),
            checkpoints,
            getSpeed(),
            getFPSLimit(),
            play->m_isPracticeMode ? "YES" : "NO"
        );

        m_infoLabel->setString(text.c_str());
    }

    void refresh() {
        for (auto const& item : m_actionSprites) {
            if (!item.second) continue;
            auto text = actionText(item.first);
            item.second->setString(text.c_str());
        }

        if (m_speedLabel) {
            auto text = fmt::format("Current Speed: {:.2f}x", getSpeed());
            m_speedLabel->setString(text.c_str());
        }

        updateInfo();
    }

    void toggleHitboxMode(int action) {
        if (action == ACT_HITBOX_ALWAYS) {
            bool value = !getHitboxesAlways();
            setBool("hitboxes-always", value);
            setBool("hitboxes", value);
            if (value) {
                setBool("player-hitbox-only", false);
                setBool("object-hitbox-only", false);
            }
        }
        else if (action == ACT_PLAYER_HITBOX_ONLY) {
            bool value = !getPlayerHitboxOnly();
            setBool("player-hitbox-only", value);
            if (value) {
                setBool("hitboxes-always", false);
                setBool("hitboxes", false);
                setBool("object-hitbox-only", false);
            }
        }
        else if (action == ACT_OBJECT_HITBOX_ONLY) {
            bool value = !getObjectHitboxOnly();
            setBool("object-hitbox-only", value);
            if (value) {
                setBool("hitboxes-always", false);
                setBool("hitboxes", false);
                setBool("player-hitbox-only", false);
            }
        }
    }

    void cycleFPS() {
        static constexpr std::array<int, 5> values = {60, 120, 144, 240, 360};
        int current = getFPSLimit();
        auto it = std::find(values.begin(), values.end(), current);
        if (it == values.end() || ++it == values.end()) {
            setInt("fps-limit", values.front());
        }
        else {
            setInt("fps-limit", *it);
        }
        applyFPSLimit();
    }

    void cycleCheckpointDelay() {
        static constexpr std::array<float, 5> values = {0.5f, 1.f, 2.f, 3.f, 5.f};
        float current = getCheckpointDelay();
        size_t index = 0;
        for (size_t i = 0; i < values.size(); ++i) {
            if (std::abs(values[i] - current) < 0.01f) {
                index = i;
                break;
            }
        }
        index = (index + 1) % values.size();
        setFloat("checkpoint-delay", values[index]);
    }

    void cyclePlayerScale() {
        static constexpr std::array<float, 6> values = {
            0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f
        };
        float current = getPlayerScaleValue();
        size_t index = 0;
        for (size_t i = 0; i < values.size(); ++i) {
            if (std::abs(values[i] - current) < 0.01f) {
                index = i;
                break;
            }
        }
        index = (index + 1) % values.size();
        setFloat("player-scale", values[index]);
    }

    bool initSamuel() {
        if (!geode::Popup::init(520.f, 330.f)) {
            return false;
        }

        this->setTitle("Samuel Mod Menu");

        auto size = m_mainLayer->getContentSize();
        float cx = size.width / 2.f;

        auto tabMenu = CCMenu::create();
        tabMenu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(tabMenu, 30);

        static constexpr std::array<char const*, TAB_COUNT> names = {
            "GAME", "HUD", "VISUAL", "TOOLS", "INFO", "FAV"
        };

        for (int i = 0; i < TAB_COUNT; ++i) {
            float x = cx - 205.f + i * 82.f;
            addTab(tabMenu, i, names[i], {x, size.height - 48.f});
        }

        for (int i = 0; i < TAB_COUNT; ++i) {
            m_pages[i] = CCMenu::create();
            m_pages[i]->setPosition({0.f, 0.f});
            m_mainLayer->addChild(m_pages[i], 10);
        }

        // GAME PAGE
        addAction(m_pages[TAB_GAME], ACT_NOCLIP, {cx - 125.f, 220.f});
        addAction(m_pages[TAB_GAME], ACT_AUTO_PRACTICE, {cx + 125.f, 220.f});
        addAction(m_pages[TAB_GAME], ACT_AUTO_RETRY, {cx - 125.f, 170.f});
        addAction(m_pages[TAB_GAME], ACT_INSTANT_RESTART, {cx + 125.f, 170.f});
        addAction(m_pages[TAB_GAME], ACT_PRACTICE_MUSIC, {cx - 125.f, 120.f});
        addAction(m_pages[TAB_GAME], ACT_GRAVITY, {cx + 125.f, 120.f});
        addAction(m_pages[TAB_GAME], ACT_TELEPORT, {cx - 125.f, 70.f});
        addAction(m_pages[TAB_GAME], ACT_RESTART, {cx + 125.f, 70.f});

        // HUD PAGE
        addAction(m_pages[TAB_HUD], ACT_FPS_LIMIT, {cx - 125.f, 220.f});
        addAction(m_pages[TAB_HUD], ACT_SHOW_FPS, {cx + 125.f, 220.f});
        addAction(m_pages[TAB_HUD], ACT_SHOW_CPS, {cx - 125.f, 170.f});
        addAction(m_pages[TAB_HUD], ACT_SHOW_ATTEMPTS, {cx + 125.f, 170.f});
        addAction(m_pages[TAB_HUD], ACT_SHOW_DEATHS, {cx - 125.f, 120.f});
        addAction(m_pages[TAB_HUD], ACT_SHOW_PERCENT, {cx + 125.f, 120.f});
        addAction(m_pages[TAB_HUD], ACT_RESET_DEATHS, {cx, 70.f}, 210.f);

        // VISUAL PAGE
        addAction(m_pages[TAB_VISUAL], ACT_NO_DEATH_FX, {cx - 125.f, 230.f});
        addAction(m_pages[TAB_VISUAL], ACT_HITBOX_ALWAYS, {cx + 125.f, 230.f});
        addAction(m_pages[TAB_VISUAL], ACT_PLAYER_HITBOX_ONLY, {cx - 125.f, 185.f});
        addAction(m_pages[TAB_VISUAL], ACT_OBJECT_HITBOX_ONLY, {cx + 125.f, 185.f});
        addAction(m_pages[TAB_VISUAL], ACT_RAINBOW_TRAIL, {cx - 125.f, 140.f});
        addAction(m_pages[TAB_VISUAL], ACT_LONG_TRAIL, {cx + 125.f, 140.f});
        addAction(m_pages[TAB_VISUAL], ACT_TRAIL, {cx - 125.f, 95.f});
        addAction(m_pages[TAB_VISUAL], ACT_HIDE_PLAYER, {cx + 125.f, 95.f});
        addAction(m_pages[TAB_VISUAL], ACT_PLAYER_SCALE, {cx, 50.f}, 220.f);

        // TOOLS PAGE
        addAction(m_pages[TAB_TOOLS], ACT_AUTO_CHECKPOINTS, {cx - 125.f, 225.f});
        addAction(m_pages[TAB_TOOLS], ACT_CHECKPOINT_DELAY, {cx + 125.f, 225.f});
        addAction(m_pages[TAB_TOOLS], ACT_REMOVE_ALL_CHECKPOINTS, {cx - 125.f, 177.f});
        addAction(m_pages[TAB_TOOLS], ACT_SLOWMO_HOTKEY, {cx + 125.f, 177.f});
        addAction(m_pages[TAB_TOOLS], ACT_SPEED_HOTKEYS, {cx - 125.f, 129.f});
        addAction(m_pages[TAB_TOOLS], ACT_PLACE_CHECKPOINT, {cx + 125.f, 129.f});
        addAction(m_pages[TAB_TOOLS], ACT_REMOVE_CHECKPOINT, {cx, 81.f}, 210.f);

        addAction(m_pages[TAB_TOOLS], ACT_SPEED_05, {cx - 150.f, 38.f}, 70.f, 0.58f);
        addAction(m_pages[TAB_TOOLS], ACT_SPEED_1, {cx - 50.f, 38.f}, 70.f, 0.58f);
        addAction(m_pages[TAB_TOOLS], ACT_SPEED_2, {cx + 50.f, 38.f}, 70.f, 0.58f);
        addAction(m_pages[TAB_TOOLS], ACT_SPEED_4, {cx + 150.f, 38.f}, 70.f, 0.58f);

        m_speedLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_speedLabel->setScale(0.34f);
        m_speedLabel->setPosition({cx, 14.f});
        m_pages[TAB_TOOLS]->addChild(m_speedLabel);

        // INFO PAGE
        m_infoLabel = CCLabelBMFont::create("", "chatFont.fnt");
        m_infoLabel->setScale(0.55f);
        m_infoLabel->setPosition({cx, 145.f});
        m_pages[TAB_INFO]->addChild(m_infoLabel);

        // FAVORITES PAGE
        addAction(m_pages[TAB_FAVORITES], ACT_NOCLIP, {cx - 125.f, 220.f});
        addAction(m_pages[TAB_FAVORITES], ACT_INSTANT_RESTART, {cx + 125.f, 220.f});
        addAction(m_pages[TAB_FAVORITES], ACT_HITBOX_ALWAYS, {cx - 125.f, 170.f});
        addAction(m_pages[TAB_FAVORITES], ACT_RAINBOW_TRAIL, {cx + 125.f, 170.f});
        addAction(m_pages[TAB_FAVORITES], ACT_SPEED_1, {cx - 125.f, 120.f});
        addAction(m_pages[TAB_FAVORITES], ACT_SPEED_2, {cx + 125.f, 120.f});
        addAction(m_pages[TAB_FAVORITES], ACT_TELEPORT, {cx - 125.f, 70.f});
        addAction(m_pages[TAB_FAVORITES], ACT_RESTART, {cx + 125.f, 70.f});

        showTab(TAB_GAME);
        refresh();
        return true;
    }

    void onTab(CCObject* sender) {
        auto node = static_cast<CCNode*>(sender);
        if (!node) return;
        showTab(node->getTag());
    }

    void onAction(CCObject* sender) {
        auto node = static_cast<CCNode*>(sender);
        if (!node) return;

        int action = node->getTag();
        auto play = PlayLayer::get();

        switch (action) {
            case ACT_NOCLIP:
                setBool("noclip", !getNoclip());
                break;

            case ACT_AUTO_PRACTICE:
                setBool("auto-practice", !getAutoPractice());
                if (play) play->togglePracticeMode(getAutoPractice());
                break;

            case ACT_AUTO_RETRY:
                setBool("auto-retry", !getAutoRetry());
                break;

            case ACT_INSTANT_RESTART:
                setBool("instant-restart", !getInstantRestart());
                break;

            case ACT_PRACTICE_MUSIC:
                setBool("practice-music", !getPracticeMusic());
                if (play && play->m_isPracticeMode) {
                    play->toggleMusicInPractice();
                }
                break;

            case ACT_GRAVITY:
                if (play) {
                    flipPlayerGravity(play->m_player1);
                    if (play->m_gameState.m_isDualMode) {
                        flipPlayerGravity(play->m_player2);
                    }
                }
                break;

            case ACT_TELEPORT:
                if (play) {
                    teleportPlayer(play->m_player1, 100.f);
                    if (play->m_gameState.m_isDualMode) {
                        teleportPlayer(play->m_player2, 100.f);
                    }
                }
                break;

            case ACT_RESTART:
                if (play) {
                    this->onClose(nullptr);
                    play->resetLevel();
                    return;
                }
                break;

            case ACT_FPS_LIMIT:
                cycleFPS();
                break;

            case ACT_SHOW_FPS:
                setBool("show-fps", !getShowFPS());
                break;

            case ACT_SHOW_CPS:
                setBool("show-cps", !getShowCPS());
                break;

            case ACT_SHOW_ATTEMPTS:
                setBool("show-attempts", !getShowAttempts());
                break;

            case ACT_SHOW_DEATHS:
                setBool("show-deaths", !getShowDeaths());
                break;

            case ACT_SHOW_PERCENT:
                setBool("show-percent", !getShowPercent());
                break;

            case ACT_RESET_DEATHS:
                g_resetDeathsRequested = true;
                break;

            case ACT_NO_DEATH_FX:
                setBool("no-death-fx", !getNoDeathFX());
                break;

            case ACT_HITBOX_ALWAYS:
            case ACT_PLAYER_HITBOX_ONLY:
            case ACT_OBJECT_HITBOX_ONLY:
                toggleHitboxMode(action);
                break;

            case ACT_RAINBOW_TRAIL:
                setBool("rainbow-trail", !getRainbowTrail());
                break;

            case ACT_LONG_TRAIL:
                setBool("long-trail", !getLongTrail());
                break;

            case ACT_TRAIL:
                setBool("trail-enabled", !getTrailEnabled());
                applyTrailVisibility(play);
                break;

            case ACT_HIDE_PLAYER:
                setBool("hide-player", !getHidePlayer());
                break;

            case ACT_PLAYER_SCALE:
                cyclePlayerScale();
                break;

            case ACT_AUTO_CHECKPOINTS:
                setBool("auto-checkpoints", !getAutoCheckpoints());
                break;

            case ACT_CHECKPOINT_DELAY:
                cycleCheckpointDelay();
                break;

            case ACT_REMOVE_ALL_CHECKPOINTS:
                if (play) play->removeAllCheckpoints();
                break;

            case ACT_SLOWMO_HOTKEY:
                setBool("slowmo-hotkey", !getSlowMoHotkey());
                if (!getSlowMoHotkey()) {
                    g_slowMoHeld = false;
                    applySpeed();
                }
                break;

            case ACT_SPEED_HOTKEYS:
                setBool("speed-hotkeys", !getSpeedHotkeys());
                break;

            case ACT_PLACE_CHECKPOINT:
                if (play) {
                    this->onClose(nullptr);
                    if (!play->m_isPracticeMode) {
                        play->togglePracticeMode(true);
                    }
                    play->createCheckpoint();
                    return;
                }
                break;

            case ACT_REMOVE_CHECKPOINT:
                if (play) {
                    this->onClose(nullptr);
                    play->removeCheckpoint(false);
                    return;
                }
                break;

            case ACT_SPEED_05:
                setSpeed(0.5f);
                break;

            case ACT_SPEED_1:
                setSpeed(1.f);
                break;

            case ACT_SPEED_2:
                setSpeed(2.f);
                break;

            case ACT_SPEED_4:
                setSpeed(4.f);
                break;
        }

        refresh();
    }

    void onClose(CCObject* sender) override {
        g_samuelPopup = nullptr;

        if (g_menuPausedGame) {
            if (auto play = PlayLayer::get()) {
                play->m_isPaused = false;
                play->resumeSchedulerAndActions();
            }
            g_menuPausedGame = false;
            applySpeed();
        }

        geode::Popup::onClose(sender);
    }

public:
    static SamuelPopup* create() {
        auto ret = new SamuelPopup();
        if (ret && ret->initSamuel()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    void closeMenu() {
        this->onClose(nullptr);
    }
};

// =====================================================
// OPEN / CLOSE MENU
// =====================================================

static void toggleSamuelMenu() {
    if (g_samuelPopup && g_samuelPopup->getParent()) {
        g_samuelPopup->closeMenu();
        return;
    }

    auto popup = SamuelPopup::create();
    if (!popup) return;

    if (auto play = PlayLayer::get()) {
        if (!play->m_isPaused) {
            play->m_isPaused = true;
            play->pauseSchedulerAndActions();
            g_menuPausedGame = true;
        }
    }

    g_samuelPopup = popup;
    popup->show();
}

// =====================================================
// KEYBOARD HOTKEYS
// M menu
// S hold = slow motion
// Q/W/E/R = 0.5/1/2/4x
// =====================================================

class $modify(SamuelKeyboardHook, cocos2d::CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(
        cocos2d::enumKeyCodes key,
        bool down,
        bool repeat,
        double timestamp
    ) {
        if (down && !repeat && key == cocos2d::KEY_M) {
            toggleSamuelMenu();
            return true;
        }

        if (getSlowMoHotkey() && key == cocos2d::KEY_S) {
            g_slowMoHeld = down;
            applySpeed();
            return true;
        }

        if (getSpeedHotkeys() && down && !repeat) {
            if (key == cocos2d::KEY_Q) {
                setSpeed(0.5f);
                return true;
            }
            if (key == cocos2d::KEY_W) {
                setSpeed(1.f);
                return true;
            }
            if (key == cocos2d::KEY_E) {
                setSpeed(2.f);
                return true;
            }
            if (key == cocos2d::KEY_R) {
                setSpeed(4.f);
                return true;
            }
        }

        return cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG(
            key,
            down,
            repeat,
            timestamp
        );
    }
};

// =====================================================
// CPS INPUT TRACKING
// =====================================================

class $modify(SamuelInputHook, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        if (
            down &&
            PlayLayer::get() &&
            static_cast<GJBaseGameLayer*>(PlayLayer::get()) == this
        ) {
            g_clickTimes.push_back(nowSeconds());
        }
    }
};

// =====================================================
// NO DEATH EFFECT
// =====================================================

class $modify(SamuelPlayerObject, PlayerObject) {
    void playDeathEffect() {
        if (getNoDeathFX()) {
            return;
        }
        PlayerObject::playDeathEffect();
    }
};

// =====================================================
// SETTINGS BUTTON
// =====================================================

class $modify(SamuelOptionsLayer, OptionsLayer) {
    void customSetup() {
        OptionsLayer::customSetup();

        applyFPSLimit();

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
            0.f
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
        toggleSamuelMenu();
    }
};

// =====================================================
// PLAY LAYER
// =====================================================

class $modify(SamuelPlayLayer, PlayLayer) {
    struct Fields {
        bool m_restartQueued = false;
        float m_checkpointTimer = 0.f;
        double m_lastFrameTime = 0.0;
        float m_fps = 0.f;
        int m_deaths = 0;
        CCDrawNode* m_playerHitboxNode = nullptr;

        CCLabelBMFont* m_fpsLabel = nullptr;
        CCLabelBMFont* m_cpsLabel = nullptr;
        CCLabelBMFont* m_attemptLabel2 = nullptr;
        CCLabelBMFont* m_deathLabel = nullptr;
        CCLabelBMFont* m_percentLabel2 = nullptr;
    };

    CCLabelBMFont* makeHudLabel(float y) {
        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setScale(0.35f);
        label->setAnchorPoint({0.f, 1.f});
        label->setPosition({8.f, y});
        if (m_uiLayer) {
            m_uiLayer->addChild(label, 1000);
        }
        else {
            this->addChild(label, 1000);
        }
        return label;
    }

    void applyPlayerVisuals(PlayerObject* player) {
        if (!player) return;

        player->setVisible(!getHidePlayer());
        player->setScale(getPlayerScaleValue());

        if (player->m_regularTrail) {
            player->m_regularTrail->setVisible(getTrailEnabled());

            if (getLongTrail()) {
                player->m_regularTrail->m_fFadeDelta = 0.12f;
                player->m_alwaysShowStreak = true;
            }
            else {
                player->m_regularTrail->m_fFadeDelta = 1.f;
                player->m_alwaysShowStreak = false;
            }

            if (getRainbowTrail()) {
                double t = nowSeconds() * 4.0;
                auto color = ccc3(
                    static_cast<GLubyte>((std::sin(t) + 1.0) * 127.5),
                    static_cast<GLubyte>((std::sin(t + 2.094) + 1.0) * 127.5),
                    static_cast<GLubyte>((std::sin(t + 4.188) + 1.0) * 127.5)
                );
                player->m_regularTrail->tintWithColor(color);
            }
        }

        if (player->m_waveTrail) {
            player->m_waveTrail->setVisible(getTrailEnabled());
        }
    }

    void drawOnePlayerHitbox(PlayerObject* player) {
        if (!player || !m_fields->m_playerHitboxNode) return;

        auto rect = player->getObjectRect();
        auto color = ccc4f(0.f, 1.f, 0.f, 1.f);

        CCPoint a = {rect.getMinX(), rect.getMinY()};
        CCPoint b = {rect.getMaxX(), rect.getMinY()};
        CCPoint c = {rect.getMaxX(), rect.getMaxY()};
        CCPoint d = {rect.getMinX(), rect.getMaxY()};

        m_fields->m_playerHitboxNode->drawSegment(a, b, 1.f, color);
        m_fields->m_playerHitboxNode->drawSegment(b, c, 1.f, color);
        m_fields->m_playerHitboxNode->drawSegment(c, d, 1.f, color);
        m_fields->m_playerHitboxNode->drawSegment(d, a, 1.f, color);
    }

    void updateHitboxes() {
        if (m_fields->m_playerHitboxNode) {
            m_fields->m_playerHitboxNode->clear();
        }

        if (getPlayerHitboxOnly()) {
            if (m_isDebugDrawEnabled) {
                this->toggleDebugDraw();
            }

            if (m_fields->m_playerHitboxNode) {
                m_fields->m_playerHitboxNode->setVisible(true);
                drawOnePlayerHitbox(m_player1);
                if (m_gameState.m_isDualMode) {
                    drawOnePlayerHitbox(m_player2);
                }
            }
            return;
        }

        if (m_fields->m_playerHitboxNode) {
            m_fields->m_playerHitboxNode->setVisible(false);
        }

        bool wantDebug = getHitboxesAlways() || getObjectHitboxOnly();

        if (m_isDebugDrawEnabled != wantDebug) {
            this->toggleDebugDraw();
        }

        m_disablePlayerHitbox = getObjectHitboxOnly();

        if (m_debugDrawNode) {
            m_debugDrawNode->setVisible(wantDebug);
        }
    }

    void updateHud() {
        auto fields = m_fields.self();

        if (g_resetDeathsRequested) {
            fields->m_deaths = 0;
            g_resetDeathsRequested = false;
        }

        double now = nowSeconds();
        while (!g_clickTimes.empty() && now - g_clickTimes.front() > 1.0) {
            g_clickTimes.pop_front();
        }

        if (fields->m_fpsLabel) {
            fields->m_fpsLabel->setVisible(getShowFPS());
            if (getShowFPS()) {
                auto text = fmt::format("FPS: {:.0f}", fields->m_fps);
                fields->m_fpsLabel->setString(text.c_str());
            }
        }

        if (fields->m_cpsLabel) {
            fields->m_cpsLabel->setVisible(getShowCPS());
            if (getShowCPS()) {
                auto text = fmt::format("CPS: {}", g_clickTimes.size());
                fields->m_cpsLabel->setString(text.c_str());
            }
        }

        if (fields->m_attemptLabel2) {
            fields->m_attemptLabel2->setVisible(getShowAttempts());
            if (getShowAttempts()) {
                auto text = fmt::format("Attempts: {}", m_attempts);
                fields->m_attemptLabel2->setString(text.c_str());
            }
        }

        if (fields->m_deathLabel) {
            fields->m_deathLabel->setVisible(getShowDeaths());
            if (getShowDeaths()) {
                auto text = fmt::format("Deaths: {}", fields->m_deaths);
                fields->m_deathLabel->setString(text.c_str());
            }
        }

        if (fields->m_percentLabel2) {
            fields->m_percentLabel2->setVisible(getShowPercent());
            if (getShowPercent()) {
                auto text = fmt::format("{:.2f}%", this->getCurrentPercent());
                fields->m_percentLabel2->setString(text.c_str());
            }
        }
    }

    bool init(
        GJGameLevel* level,
        bool useReplay,
        bool dontCreateObjects
    ) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        auto fields = m_fields.self();

        fields->m_restartQueued = false;
        fields->m_checkpointTimer = 0.f;
        fields->m_lastFrameTime = nowSeconds();
        fields->m_fps = 0.f;
        fields->m_deaths = 0;

        g_clickTimes.clear();
        g_slowMoHeld = false;

        applyFPSLimit();
        applySpeed();

        if (getAutoPractice() && !m_isPracticeMode) {
            this->togglePracticeMode(true);
        }

        if (getPracticeMusic() && m_isPracticeMode) {
            this->toggleMusicInPractice();
        }

        auto win = CCDirector::get()->getWinSize();
        fields->m_fpsLabel = makeHudLabel(win.height - 8.f);
        fields->m_cpsLabel = makeHudLabel(win.height - 25.f);
        fields->m_attemptLabel2 = makeHudLabel(win.height - 42.f);
        fields->m_deathLabel = makeHudLabel(win.height - 59.f);
        fields->m_percentLabel2 = makeHudLabel(win.height - 76.f);

        fields->m_playerHitboxNode = CCDrawNode::create();
        if (auto objectLayer = this->getObjectLayer()) {
            objectLayer->addChild(fields->m_playerHitboxNode, 9999);
        }
        else {
            this->addChild(fields->m_playerHitboxNode, 9999);
        }

        applyTrailVisibility(this);
        updateHitboxes();
        updateHud();

        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (getNoclip() && !m_levelEndAnimationStarted) {
            return;
        }

        bool realDeath =
            player &&
            !player->m_isDead &&
            object != m_anticheatSpike &&
            !m_levelEndAnimationStarted;

        PlayLayer::destroyPlayer(player, object);

        if (!realDeath || !player || !player->m_isDead) {
            return;
        }

        auto fields = m_fields.self();
        fields->m_deaths += 1;

        if (getInstantRestart()) {
            fields->m_restartQueued = true;
        }
        else if (getAutoRetry()) {
            this->delayedResetLevel();
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto fields = m_fields.self();

        if (g_resetDeathsRequested) {
            fields->m_deaths = 0;
            g_resetDeathsRequested = false;
        }

        double now = nowSeconds();
        double frameTime = now - fields->m_lastFrameTime;
        if (frameTime > 0.000001 && frameTime < 1.0) {
            float instantFPS = static_cast<float>(1.0 / frameTime);
            if (fields->m_fps <= 0.f) {
                fields->m_fps = instantFPS;
            }
            else {
                fields->m_fps = fields->m_fps * 0.9f + instantFPS * 0.1f;
            }
        }
        fields->m_lastFrameTime = now;

        if (fields->m_restartQueued) {
            fields->m_restartQueued = false;
            this->resetLevel();
            return;
        }

        applyPlayerVisuals(m_player1);
        if (m_gameState.m_isDualMode) {
            applyPlayerVisuals(m_player2);
        }

        updateHitboxes();
        updateHud();

        if (
            getAutoCheckpoints() &&
            m_isPracticeMode &&
            m_player1 &&
            !m_player1->m_isDead &&
            !m_levelEndAnimationStarted
        ) {
            fields->m_checkpointTimer += dt;

            if (fields->m_checkpointTimer >= getCheckpointDelay()) {
                fields->m_checkpointTimer = 0.f;
                this->createCheckpoint();
            }
        }
        else {
            fields->m_checkpointTimer = 0.f;
        }
    }
};
