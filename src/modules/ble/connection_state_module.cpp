#include "connection_state_module.h"
#include "ble_client.h"
#include "packet_parser.h"
#include "display.h"
#include "../include/display_layout.h"
#include "modules/power/power_module.h"
#include "modules/ble/ble_queue_module.h"
#include "modules/system/system_event_bus.h"
#include "modules/display/dashboard_module.h"

#define CONN_LOG(...) do { } while(0)

void ConnectionStateModule::begin(V1BLEClient* bleClient,
                                  PacketParser* parserPtr,
                                  V1Display* displayPtr,
                                  PowerModule* powerModule,
                                  BleQueueModule* bleQueueModule,
                                  SystemEventBus* eventBus,
                                  DashboardModule* dashboard) {
    ble_ = bleClient;
    parser_ = parserPtr;
    display_ = displayPtr;
    power_ = powerModule;
    bleQueue_ = bleQueueModule;
    bus_ = eventBus;
    dashboard_ = dashboard;
    wasConnected_ = false;
    lastDataRequestMs_ = 0;
}

bool ConnectionStateModule::process(unsigned long nowMs) {
    if (!ble_ || !parser_ || !display_) {
        return false;
    }

    bool isConnected = ble_->isConnected();

    // Handle state transitions
    if (isConnected != wasConnected_) {
        if (power_) {
            power_->onV1ConnectionChange(isConnected);
        }

        if (isConnected) {
            // Just connected
            display_->showResting();
            CONN_LOG("[BLE] V1 connected");
            if (bus_) {
                SystemEvent event;
                event.type = SystemEventType::BLE_CONNECTED;
                event.tsMs = static_cast<uint32_t>(nowMs);
                bus_->publish(event);
            }
        } else {
            // Just disconnected - reset stale state
            PacketParser::resetAlertCountTracker();
            parser_->resetAlertAssembly();
            V1Display::resetChangeTracking();
            // If an idle screen is active, let it keep rendering rather than
            // overwriting it with the scanning screen immediately.
            if (!dashboard_ || !dashboard_->isActive()) {
                display_->showScanning();
            }
            CONN_LOG("[BLE] V1 disconnected - scanning");
            if (bus_) {
                SystemEvent event;
                event.type = SystemEventType::BLE_DISCONNECTED;
                event.tsMs = static_cast<uint32_t>(nowMs);
                bus_->publish(event);
            }
        }
        wasConnected_ = isConnected;
    }

    // If connected but not seeing traffic, periodically re-request alert data
    if (isConnected && bleQueue_) {
        unsigned long lastRx = bleQueue_->getLastRxMillis();
        bool dataStale = (nowMs - lastRx) > DATA_STALE_MS;
        bool canRequest = (nowMs - lastDataRequestMs_) > DATA_REQUEST_INTERVAL_MS;

        if (dataStale && canRequest) {
            ble_->requestAlertData();
            lastDataRequestMs_ = nowMs;
        }
    }

    // When disconnected, either render the active idle screen or refresh
    // the scanning indicators.  The display pipeline only runs when BLE
    // packets arrive (parsedReady), so idle screens must be driven here.
    if (!isConnected) {
        if (dashboard_ && dashboard_->isActive()) {
            dashboard_->renderIfActive(static_cast<uint32_t>(nowMs));
        } else {
            display_->drawWiFiIndicator();
            display_->drawBatteryIndicator();
        }
    }

    return isConnected;
}
