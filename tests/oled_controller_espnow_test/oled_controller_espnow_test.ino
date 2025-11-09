#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Bluepad32.h>
#include <esp_now.h>
#include <WiFi.h>

// --- OLED Configuration ---
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 27
#define OLED_SCLK 26

// --- Bluepad32 Configuration ---
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// OLED Display
Adafruit_SH1106G display(128, 64, &Wire, -1);

// Controller data buffer (we'll send this over ESP-NOW)
uint8_t controllerData[16]; // 16 bytes should be enough for 1 controller

// --- ESP-NOW Peer MAC (broadcast in this example) ---
uint8_t broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// --------------------------------------------------------------------------
//                               OLED FUNCTIONS
// --------------------------------------------------------------------------
void screenSetup() {
    Wire.begin(OLED_SDA, OLED_SCLK);
    if (!display.begin(SCREEN_ADDRESS)) {
        Serial.println(F("SH1106 allocation failed!"));
        for(;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Bluepad32 Init...");
    display.display();
    delay(1000);
}

void screenUpdate(const char* s) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println(s);
    display.display();
}

// --------------------------------------------------------------------------
//                            BLUEPAD32 FUNCTIONS
// --------------------------------------------------------------------------
void dumpControllerData(ControllerPtr ctl) {
    // Clear buffer
    memset(controllerData, 0, sizeof(controllerData));

    // Byte 0: D-Pad + Buttons (bitmask)
    uint8_t dpad = ctl->dpad();
    controllerData[0] = dpad & 0x0F; // lower 4 bits for DPAD

    // Other buttons: X=bit4, B=bit5, Share=bit6, Options=bit7
    if (ctl->x()) controllerData[0] |= 1 << 4;
    if (ctl->b()) controllerData[0] |= 1 << 5;
    if (ctl->share()) controllerData[0] |= 1 << 6;
    if (ctl->options()) controllerData[0] |= 1 << 7;

    // Bytes 1-2: Right trigger/throttle 0-1023 -> 2 bytes
    int throttle = ctl->throttle(); // 0-1023
    controllerData[1] = throttle & 0xFF;
    controllerData[2] = (throttle >> 8) & 0xFF;

    // Bytes 3-6: Left stick X/Y (-511..512) -> 2 bytes each
    int16_t lx = ctl->axisX();
    int16_t ly = ctl->axisY();
    controllerData[3] = lx & 0xFF;
    controllerData[4] = (lx >> 8) & 0xFF;
    controllerData[5] = ly & 0xFF;
    controllerData[6] = (ly >> 8) & 0xFF;

    // Bytes 7-10: Right stick X/Y
    int16_t rx = ctl->axisRX();
    int16_t ry = ctl->axisRY();
    controllerData[7] = rx & 0xFF;
    controllerData[8] = (rx >> 8) & 0xFF;
    controllerData[9] = ry & 0xFF;
    controllerData[10] = (ry >> 8) & 0xFF;

    // Byte 11: controller index
    controllerData[11] = ctl->index();

    // Send over ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, controllerData, sizeof(controllerData));
    if (result == ESP_OK) {
        Serial.println("ESP-NOW Send OK");
    } else {
        Serial.println("ESP-NOW Send Fail");
    }
}

void processControllers() {
    for (auto ctl : myControllers) {
        if (ctl && ctl->isConnected() && ctl->hasData()) {
            if (ctl->isGamepad()) {
                dumpControllerData(ctl);
            }
        }
    }
}

// Callbacks
void onConnectedController(ControllerPtr ctl) {
    for (int i=0;i<BP32_MAX_GAMEPADS;i++) {
        if (myControllers[i]==nullptr) {
            Serial.printf("Controller connected, index=%d\n", i);
            myControllers[i]=ctl;
            break;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i=0;i<BP32_MAX_GAMEPADS;i++) {
        if (myControllers[i]==ctl) {
            Serial.printf("Controller disconnected, index=%d\n", i);
            myControllers[i]=nullptr;
            break;
        }
    }
}

// --------------------------------------------------------------------------
//                             ARDUINO FUNCTIONS
// --------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    screenSetup();

    // --- Bluepad32 setup ---
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);

    // --- ESP-NOW setup ---
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    screenUpdate("Connect Gamepad...");
}

void loop() {
    if (BP32.update()) {
        processControllers();
    }
    delay(50); // 20Hz update
}
