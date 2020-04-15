#include <OneWire.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels


#define SAMPLING_RATE 1.0 //Sampling rate in Hz (can't be faster than 1 Hz). Smaller than 1 Hz values are allowed
#define SAMPLING_PERIOD (long)((1.0/SAMPLING_RATE)*1000) // Sampling period (miliseconds)

//GPIOs
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define TEMP_SENSOR    3 // One-wire pin for temperature sensor DS18B20
#define TEMP_VCC       2 // Power supply for low-power temp sensor

//Prototypes
float getTemperature();
void displayTemp(float temperature);




//OLED object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Temp sensor object init
OneWire ds(TEMP_SENSOR);


//Globals for temperature sensor
byte i;
byte present = 0;
byte type_s;
byte data[12];
byte addr[8];
float celsius, fahrenheit;

//Globals for precise time measurement
long currentTime;
long lastTime;

float getTemperature(){
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); //Start temperature aqcuisition and conversion

  delay(800);

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);    //Read data from sensor's scratchpad
  for ( i = 0; i < 9; i++) { //9 bytes read from scratchpad
    data[i] = ds.read();
  }
  
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;

  return celsius;
}


void displayTemp(float temperature){
  display.clearDisplay();

  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.cp437(true); //Use full 256 char 'Code Page 437' font

  display.print(temperature);
  display.setTextSize(1);
  display.print(" ");
  display.print("o");
  display.setTextSize(2);
  display.print("C");

  display.display();
}


void setup() {

  // OLED Initialization
  
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  pinMode(TEMP_VCC, OUTPUT);
  digitalWrite(TEMP_VCC, 1);

  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();


  //Temperature sensor (DS18B20) intialization

  
  if(!ds.search(addr)){
    ds.reset_search();
    return;
  }

  currentTime = micros();
  lastTime = currentTime;

}

void loop() {
  volatile float temp;
  
  temp = getTemperature();
  displayTemp(temp);
  Serial.println(temp);

  
  //Improving precision of sampling rate
  do{
    currentTime = micros();    
  }while(SAMPLING_PERIOD*1000 > (currentTime - lastTime));
  lastTime = micros();
}
