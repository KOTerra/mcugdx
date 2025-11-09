#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

// --- OLED Configuration from Original Code ---
#define OLED_RESET -1 // Used in Adafruit library but generally -1 for ESP32
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 27 // ESP32 pin for I2C Data
#define OLED_SCLK 26 // ESP32 pin for I2C Clock
// ---------------------------------------------

// Create the display object, specifying the I2C pins (SDA, SCL)
Adafruit_SH1106 display(OLED_SDA, OLED_SCLK);

void screenSetup() {
  // Initialize I2C communication with the display
  // SH1106_SWITCHCAPVCC is used to generate the display voltage internally
  display.begin(SH1106_SWITCHCAPVCC, SCREEN_ADDRESS); 

  // Clear the buffer
  display.clearDisplay();
}

void printHelloWorld() {
  // Clear the display buffer again before writing
  display.clearDisplay();

  // Set text properties
  display.setTextSize(1);        // Smallest size (6x8 pixels per character)
  display.setTextColor(WHITE);   // Draw text in white
  display.setCursor(0, 0);       // Start at the top-left corner (x=0, y=0)

  // Print the text to the display buffer
  display.println("Hello World!");
  
  // Set larger text size and a different position for a second line
  display.setTextSize(2);        
  display.setCursor(0, 15);      // Move down a bit
  display.println("ESP32");

  // Display the buffer on the screen
  display.display();
}

void setup() {
  // Initialize Serial communication for debugging (optional, but good practice)
  Serial.begin(9600);
  Serial.println("OLED Test Starting...");
  
  // Setup the screen
  screenSetup();
  
  // Print "Hello World!"
  printHelloWorld();
  Serial.println("Hello World printed to OLED.");
}

void loop() {
  // Nothing needs to be done repeatedly in the loop for a static "Hello World"
}