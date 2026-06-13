#include <unity.h>

#include "../mocks/Arduino.h"
#include "../mocks/settings.h"
#include "../mocks/display.h"
#include "../mocks/ble_client.h"
#include "../mocks/packet_parser.h"
#include "../mocks/touch_handler.h"
#include "../mocks/modules/auto_push/auto_push_module.h"
#include "../mocks/modules/alert_persistence/alert_persistence_module.h"

#ifndef ARDUINO
SerialClass Serial;
unsigned long mockMillis = 0;
unsigned long mockMicros = 0;
#endif

SettingsManager settingsManager;

#include "../../src/modules/screens/screen_manager.h"
#include "../../src/modules/history/history_manager.h"
#include "../../src/modules/services/jbv1_client.h"

// Stub globals required by main_globals.h externs used in tap_gesture_module
ScreenManager screenManager;
HistoryManager historyManager;
JBV1Data g_jbv1;

// ScreenManager stubs (render() not exercised in tap gesture tests)
void ScreenManager::next() { uint8_t n = (static_cast<uint8_t>(current_) + 1) % static_cast<uint8_t>(ScreenType::COUNT); current_ = static_cast<ScreenType>(n); }
void ScreenManager::previous() { uint8_t c = static_cast<uint8_t>(current_), cnt = static_cast<uint8_t>(ScreenType::COUNT); current_ = static_cast<ScreenType>(c == 0 ? cnt - 1 : c - 1); }
void ScreenManager::set(ScreenType s) { if (static_cast<uint8_t>(s) < static_cast<uint8_t>(ScreenType::COUNT)) current_ = s; }
void ScreenManager::forceRadar() { current_ = ScreenType::RADAR; }
void ScreenManager::tick(bool hasAlert, uint32_t) { if (hasAlert && !lastHadAlert_) forceRadar(); lastHadAlert_ = hasAlert; }
void ScreenManager::render(V1Display&, const DisplayBleContext&) {}

// HistoryManager stubs
void HistoryManager::addEncounter(const char*, const char*, uint32_t, uint32_t, uint8_t) {}
void HistoryManager::clear() {}
const Encounter& HistoryManager::get(int) const { static Encounter e{}; return e; }

// JBV1Data stub
bool JBV1Data::valid() const { return false; }

#include "../../src/modules/quiet/quiet_coordinator_module.cpp"
#include "../../src/modules/touch/tap_gesture_module.cpp"

namespace {

V1Display display;
V1BLEClient bleClient;
PacketParser parser;
TouchHandler touch;
AutoPushModule autoPush;
AlertPersistenceModule alertPersistence;
DisplayMode displayMode = DisplayMode::LIVE;
QuietCoordinatorModule quiet;
TapGestureModule module;

AlertData makeAlert() {
    return AlertData::create(BAND_KA, DIR_FRONT, 4, 0, 34520, true, true);
}

void setTime(unsigned long nowMs) {
    mockMillis = nowMs;
    mockMicros = nowMs * 1000;
}

void processAt(unsigned long nowMs) {
    setTime(nowMs);
    module.process(nowMs);
}

}  // namespace

void setUp() {
    setTime(0);
    ::settingsManager = SettingsManager{};
    ::settingsManager.settings.activeSlot = 0;
    ::settingsManager.settings.autoPushEnabled = false;
    display.reset();
    bleClient.reset();
    parser.reset();
    touch.reset();
    autoPush.reset();
    alertPersistence.reset();
    displayMode = DisplayMode::LIVE;

    module = TapGestureModule{};
    quiet.begin(&bleClient, &parser);
    module.begin(
        &touch,
        &::settingsManager,
        &display,
        &bleClient,
        &parser,
        &autoPush,
        &alertPersistence,
        &displayMode,
        &quiet);
}

void tearDown() {}

void test_alert_tap_toggles_mute_immediately() {
    parser.setAlerts({makeAlert()});
    parser.state.muted = false;
    touch.queueTouch(40, 20);

    processAt(200);

    TEST_ASSERT_EQUAL(1, bleClient.setMuteCalls);
    TEST_ASSERT_TRUE(bleClient.lastMuteValue);
    TEST_ASSERT_EQUAL(0, autoPush.queueSlotPushCalls);
    TEST_ASSERT_EQUAL(0, display.drawProfileIndicatorCalls);
}

void test_idle_triple_tap_cycles_slot_and_pushes_when_connected() {
    ::settingsManager.settings.autoPushEnabled = true;
    bleClient.setConnected(true);
    touch.queueTouch(10, 10);
    touch.queueTouch(10, 10);
    touch.queueTouch(10, 10);

    processAt(200);
    TEST_ASSERT_EQUAL(0, display.drawProfileIndicatorCalls);
    TEST_ASSERT_EQUAL(0, autoPush.queueSlotPushCalls);

    processAt(400);
    TEST_ASSERT_EQUAL(0, display.drawProfileIndicatorCalls);
    TEST_ASSERT_EQUAL(0, autoPush.queueSlotPushCalls);

    processAt(600);

    TEST_ASSERT_EQUAL(1, ::settingsManager.settings.activeSlot);
    TEST_ASSERT_EQUAL(DisplayMode::IDLE, displayMode);
    TEST_ASSERT_EQUAL(1, alertPersistence.clearPersistenceCalls);
    TEST_ASSERT_EQUAL(1, display.drawProfileIndicatorCalls);
    TEST_ASSERT_EQUAL(1, display.lastProfileIndicatorSlot);
    TEST_ASSERT_EQUAL(1, autoPush.queueSlotPushCalls);
    TEST_ASSERT_EQUAL(1, autoPush.lastQueueSlotPushSlot);
}

void test_idle_profile_cycle_resets_after_tap_window_expires() {
    touch.queueTouch(10, 10);
    touch.queueTouch(10, 10);
    touch.queueTouch(10, 10);

    processAt(200);
    processAt(400);
    processAt(1201);

    TEST_ASSERT_EQUAL(0, display.drawProfileIndicatorCalls);
    TEST_ASSERT_EQUAL(0, autoPush.queueSlotPushCalls);
    TEST_ASSERT_EQUAL(0, ::settingsManager.settings.activeSlot);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_alert_tap_toggles_mute_immediately);
    RUN_TEST(test_idle_triple_tap_cycles_slot_and_pushes_when_connected);
    RUN_TEST(test_idle_profile_cycle_resets_after_tap_window_expires);
    return UNITY_END();
}
