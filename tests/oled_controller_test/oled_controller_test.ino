#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> // Correct library for SH1106/SH1107
#include <Bluepad32.h>

// --- OLED Configuration ---
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 27
#define OLED_SCLK 26

// --- Bluepad32 Configuration ---
// Max number of controllers supported
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Correct display instantiation using Adafruit_SH110X
// Constructor: width, height, &Wire, reset_pin
Adafruit_SH1106G display(128, 64, &Wire, -1);

// Global string to hold data for the OLED
String displayData = "";

// --------------------------------------------------------------------------
//                               OLED FUNCTIONS
// --------------------------------------------------------------------------
void screenSetup() {
  // Initialize I2C with custom pins
  Wire.begin(OLED_SDA, OLED_SCLK);

  // Initialize the display
  if (!display.begin(SCREEN_ADDRESS)) {
     Serial.println(F("SH1106 allocation failed or not found!"));
     for(;;); // Halt if display init fails
  }
  
  display.clearDisplay();
  
  // Display a quick boot message
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

// Function to format and output the selected gamepad data
void dumpSelectedGamepadData(ControllerPtr ctl) {
    uint8_t dpad_value = ctl->dpad();
    String dpad_str = "";
    if (dpad_value & DPAD_UP) dpad_str += "U";
    if (dpad_value & DPAD_DOWN) dpad_str += "D";
    if (dpad_value & DPAD_LEFT) dpad_str += "L";
    if (dpad_value & DPAD_RIGHT) dpad_str += "R";
    if (dpad_str == "") dpad_str = "-";
    
    bool x_pressed = ctl->a(); 
    int throttle_value = ctl->throttle(); 
    int left_stick_x = ctl->axisX();
    int left_stick_y = ctl->axisY();
    int right_stick_x = ctl->axisRX();
    int right_stick_y = ctl->axisRY();

    displayData = "C" + String(ctl->index());
    displayData += " DPAD: " + dpad_str;
    displayData += "\nX: " + String(x_pressed ? "ON" : "OFF") + " | R2: " + String(throttle_value);
    displayData += "\n\nL-Stick:";
    displayData += "\nX:" + String(left_stick_x) + " Y:" + String(left_stick_y);
    displayData += "\n\nR-Stick:";
    displayData += "\nX:" + String(right_stick_x) + " Y:" + String(right_stick_y);

    Serial.println(displayData);
}

void processGamepad(ControllerPtr ctl) {
    dumpSelectedGamepadData(ctl);
}

// Callbacks for controller connection/disconnection
void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("CALLBACK: Controller connected, index=%d\n", i);
            myControllers[i] = ctl;
            break;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            displayData = "Controller " + String(i) + "\nDisconnected";
            screenUpdate(displayData.c_str());
            break;
        }
    }
}

// Iterate through controllers and process data
void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            }
        }
    }
}

// --------------------------------------------------------------------------
//                             ARDUINO FUNCTIONS
// --------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    screenSetup();

    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);

    Serial.println("Ready to connect Gamepad...");
    displayData = "Connect Gamepad...";
    screenUpdate(displayData.c_str());
}

void loop() {
    bool dataUpdated = BP32.update();
    
    if (dataUpdated) {
        processControllers();
        if (displayData.startsWith("C")) {
             screenUpdate(displayData.c_str());
        }
    }
    
    delay(150); 
}
