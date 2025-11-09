#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> 
#include <Bluepad32.h>

// --- Button Pin Definitions ---
// Define the GPIO pins to be used as digital outputs for button imitation.
#define PIN_FIRE    18
#define PIN_USE     19
#define PIN_ESCAPE  21
#define PIN_ENTER   22
#define PIN_RIGHT   23
#define PIN_DOWN    25
#define PIN_LEFT    32 
#define PIN_UP      33

// --- OLED Configuration ---
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 27 // Custom SDA pin
#define OLED_SCLK 26 // Custom SCLK pin

// --- Bluepad32 Configuration ---
// Max number of controllers supported
ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Correct display instantiation using Adafruit_SH110X (for SH1106G/SH1107)
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
//                            GPIO CONTROL FUNCTION
// --------------------------------------------------------------------------
// Maps controller inputs to physical GPIO pins to imitate button presses.
void controlGPIO(ControllerPtr ctl) {
    // DPAD/Directional Buttons
    uint8_t dpad_value = ctl->dpad();
    
    // *** IMPORTANT: This logic assumes an "Active-Low" setup, ***
    // *** meaning a button press is represented by setting the pin LOW. ***
    // *** If your target system uses Active-High, swap LOW and HIGH below. ***

    // Directions
    digitalWrite(PIN_UP, (dpad_value & DPAD_UP) ? LOW : HIGH);
    digitalWrite(PIN_DOWN, (dpad_value & DPAD_DOWN) ? LOW : HIGH);
    digitalWrite(PIN_LEFT, (dpad_value & DPAD_LEFT) ? LOW : HIGH);
    digitalWrite(PIN_RIGHT, (dpad_value & DPAD_RIGHT) ? LOW : HIGH);

    // Other Buttons 
    digitalWrite(PIN_FIRE, ctl->a() ? LOW : HIGH);       // 'A' button -> PIN_FIRE
    digitalWrite(PIN_USE, ctl->b() ? LOW : HIGH);        // 'B' button -> PIN_USE
    digitalWrite(PIN_ENTER, ctl->miscStart() ? LOW : HIGH);  // 'Start' button -> PIN_ENTER
    digitalWrite(PIN_ESCAPE, ctl->miscSelect() ? LOW : HIGH);// 'Select' button -> PIN_ESCAPE
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
    
    bool a_pressed = ctl->a(); 
    int throttle_value = ctl->throttle(); 
    int left_stick_x = ctl->axisX();
    int left_stick_y = ctl->axisY();
    int right_stick_x = ctl->axisRX();
    int right_stick_y = ctl->axisRY();

    displayData = "C" + String(ctl->index());
    displayData += " DPAD: " + dpad_str;
    displayData += "\nA: " + String(a_pressed ? "ON" : "OFF") + " | R2: " + String(throttle_value);
    displayData += "\n\nL-Stick:";
    displayData += "\nX:" + String(left_stick_x) + " Y:" + String(left_stick_y);
    displayData += "\n\nR-Stick:";
    displayData += "\nX:" + String(right_stick_x) + " Y:" + String(right_stick_y);

    Serial.println(displayData);
}

void processGamepad(ControllerPtr ctl) {
    dumpSelectedGamepadData(ctl);
    // Call the GPIO control function here to update outputs
    controlGPIO(ctl);
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

    // --- Configure Button Pins as OUTPUTs ---
    pinMode(PIN_FIRE, OUTPUT);
    pinMode(PIN_USE, OUTPUT);
    pinMode(PIN_ESCAPE, OUTPUT);
    pinMode(PIN_ENTER, OUTPUT);
    pinMode(PIN_RIGHT, OUTPUT);
    pinMode(PIN_DOWN, OUTPUT);
    pinMode(PIN_LEFT, OUTPUT);
    pinMode(PIN_UP, OUTPUT);

    // Initialize all buttons to the "unpressed" state (HIGH for active-low)
    // This is important to prevent false button presses at startup.
    digitalWrite(PIN_FIRE, HIGH);
    digitalWrite(PIN_USE, HIGH);
    digitalWrite(PIN_ESCAPE, HIGH);
    digitalWrite(PIN_ENTER, HIGH);
    digitalWrite(PIN_RIGHT, HIGH);
    digitalWrite(PIN_DOWN, HIGH);
    digitalWrite(PIN_LEFT, HIGH);
    digitalWrite(PIN_UP, HIGH);
    // ---------------------------------------

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
        // Update the screen only if controller data was processed successfully
        if (displayData.startsWith("C")) {
             screenUpdate(displayData.c_str());
        }
    }
    

    delay(150); 
}