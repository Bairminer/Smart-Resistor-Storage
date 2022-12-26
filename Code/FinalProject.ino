//Smart Resistor Storage
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <neopixel.h>
#include <Particle.h>
// declare LED strip and OLED display
Adafruit_SSD1306 display(-1);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, D5, WS2812);
// colors
int green = strip.Color(0, 255, 0);
int red = strip.Color(255, 0, 0);
int yellow = strip.Color(255, 255, 0);
int blue = strip.Color(0, 0, 255);
// initialize pins and variables
int up = D2;
int mode = D3;
int down = D4;
int upPrev = LOW;
int modePrev = LOW;
int downPrev = LOW;
int resistors[][2] = {{5100,0}, {1000,0}, {330,0}, {33,0}, {680,0}, {10,0}, {100,0}, {470,0}, {510,0}, {4700,0}, {2200,0}, {82,0}, {68,0}, {220,0}, {820,0}, {22,0}};
int currCell = 0;
int currValue;
int currCount;
int storedVal;

void setup() {
    // setup
    // read EEPROM
    EEPROM.get(0, storedVal);
    if (storedVal == -1){  // EEPROM not initialized, set to default values (0)
        for (int i = 0; i < 16; i++){
            EEPROM.put(i * 8, resistors[i][1]);
        }
    }
    for (int i = 0; i < 16; i++){ // read from EEPROM into resistors[] array
        EEPROM.get(i * 8, storedVal);
        resistors[i][1] = storedVal;
    }
    // set button inputs
    pinMode(up, INPUT_PULLUP);
    pinMode(mode, INPUT_PULLUP);
    pinMode(down, INPUT_PULLUP);
    // Cloud variables and functions
    Particle.variable("currValue", currValue);
    Particle.variable("currCount", currCount);
    Particle.function("selectValue", selectValue);
    Particle.function("changeCount", changeCount);
    // Set display options
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.setRotation(2);
    display.display();
    delay(2000); 
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    // Start LED strip
    strip.begin();
    // set currently selected to first cell in resistors[] array
    currValue = resistors[currCell][0];
    currCount = resistors[currCell][1];
}

int selectValue(String value){
    // selects resistor
    int inputValue = value.toInt();
    for (int i = 0; i < 16; i++){
        if (resistors[i][0] == inputValue){ // search for correct value
            currCell = i;
            currValue = resistors[currCell][0];
            currCount = resistors[currCell][1];
            return 0;
        }
    }
    return -1;
}

int changeCount(String increment){
    // add/subtracts to selected
    int incrementValue = increment.toInt();
    if (resistors[currCell][1] + incrementValue >= 0){ // increment if valid value is entered
        resistors[currCell][1] += incrementValue;
        currCount = resistors[currCell][1];
        if (resistors[currCell][1] < 5){ // publish event in case of low stock
            Particle.publish("Low Stock!", "Value: " + String(resistors[currCell][0]) + " Quantity: " + String(resistors[currCell][1]));
        }
    }
    return 0;
}

void setLED(){
    // checks quantities of each cell and sets LEDs to appropriate color
    for (int i = 0; i < 16; i++){
        strip.setPixelColor(i, green);
        if (resistors[i][1] <= 10){
            strip.setPixelColor(i, yellow);
        }
        if (resistors[i][1] <= 5){
            strip.setPixelColor(i, red);
        }
        if (i == currCell){
            strip.setPixelColor(i, blue);
        }
    }
    strip.show();
}

void loop() {
    setLED();
    int currMode = digitalRead(mode);
    if(currMode == HIGH && modePrev == LOW){ // change mode if button pressed
        currCell++;
        if(currCell > 15){
            currCell = 0;
        }
        currValue = resistors[currCell][0];
        currCount = resistors[currCell][1];
    }
    modePrev = currMode;

    int currUp = digitalRead(up); // increase currently selected resistor quantity if up is pressed
    if(currUp == HIGH && upPrev == LOW){
        resistors[currCell][1]++;
        currCount = resistors[currCell][1];
    }
    upPrev = currUp;

    int currDown = digitalRead(down); // decrease currently selected resistor quantity if down is pressed
    if(currDown == HIGH && downPrev == LOW){
        if (resistors[currCell][1] - 1 >= 0){ // prevent value from being negative
            resistors[currCell][1]--;
            currCount = resistors[currCell][1];
            if (resistors[currCell][1] < 5){ // publish event in case of low stock
                Particle.publish("Low Stock!", "Value: " + String(resistors[currCell][0]) + " Quantity: " + String(resistors[currCell][1]));
            }
        }
    }
    downPrev = currDown;
    // update data shown on screen
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Val:");
    display.println(resistors[currCell][0]);
    display.print("Count:");
    display.println(resistors[currCell][1]);
    display.display();
    // write any changes to quantities to EEPROM
    for (int i = 0; i < 16; i++){
        EEPROM.put(i * 8, resistors[i][1]);
    }
}