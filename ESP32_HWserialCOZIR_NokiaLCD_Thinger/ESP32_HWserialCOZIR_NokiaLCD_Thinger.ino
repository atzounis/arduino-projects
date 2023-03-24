#define _DEBUG_
#define _DISABLE_TLS_
#define THINGER_SERIAL_DEBUG
#define THINGER_SERVER "YOUR-THINGER-SERVER"
#include <cozir.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Thinger stuff
#include <ThingerESP32.h>
#include <WiFi.h>

// thinger setup
#define USERNAME "YOUR-THINGER-USERNAME"
#define DEVICE_ID "YOUR-DEVICE-ID"
#define DEVICE_CREDENTIAL "YOUR-DEVICE-CREDENTIALS"
ThingerESP32 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

#define SSID "YOUR-WIFI-SSID"
#define SSID_PASSWORD "YOUR-WIFI-PASSWORD"


// display wiring
#define CLK 18
#define DIN 23
#define DC  4
#define CE  15
#define RST 2

// setting up the display
Adafruit_PCD8544 display = Adafruit_PCD8544 (CLK,DIN,DC,CE,RST);

// setting up the softwareserial for the CO2 sensor
// SoftwareSerial sws(21, 19);// RX, TX pins on Ardunio
#define RXD2 16
#define TXD2 17
COZIR czr(&Serial2);

char receivedChar; //char c input to calibrate fresh air
boolean newData = false;

void setup()
{
  // sws.begin(9600);
  //  while (!sws) ;
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    while (!Serial2) ;
  czr.init(); // setting operation mode to polling
  
  Serial.begin(9600);
   while (!Serial) ;
  Serial.print("Test board ESP32-Wrover");
  Serial.println(COZIR_LIB_VERSION);
  Serial.println();

  delay(1000);

  // init the display
   display.begin ();
   display.setContrast (45);                        
   display.clearDisplay ();                     
   display.setRotation (2);  

   display.setTextSize (1);
   display.setTextColor (BLACK);
   display.setCursor (12,3);
   display.println ("CO2-RH%-Temp");
   display.setTextSize (2);
   display.setCursor (10,20);
   display.println ("SENSOR");
   display.display ();   

  // Thinger + Server setup
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, SSID_PASSWORD);
  thing.add_wifi(SSID, SSID_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
// Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Cleaning the faulty values when starting the sensor

  for (int i=0; i<3; i++){
    czr.celsius();
    czr.humidity();
    czr.CO2();
  }
  
  // prepare thinger message
  thing[DEVICE_ID] >> [](pson& out){
      out["temperature"] = czr.celsius();
      out["humidity"] = czr.humidity();
      out["CO2"] = czr.CO2();
    };

  thing["calibrate"] << [](pson& in){
    if(in.is_empty()){
        // Do nothing
        //
    }
    else{
        // remote command to calibrate fresh air
        czr.calibrateFreshAir();
        Serial.println("Remote calibration command...");
    }
  };
}

void loop()
{

  recvOneChar();
//   if(receivedChar=="c"&& newData==true){
//   //  czr.CalibrateFreshAir();
//  //   Serial.println("Reseting to fresh air...");
//     newData = false;
//   }
  
  float t = czr.celsius();
//  float f = czr.Fahrenheit();
  float h = czr.humidity();
  uint32_t c = czr.CO2();

  thing.handle();
 
  Serial.print("Celcius: ");    Serial.print(t);
  Serial.print(" Humidity: ");   Serial.print(h);
  Serial.print(" CO2: ");        Serial.print(c);
  Serial.println();
  
  display.begin ();
  display.setContrast (45);                        
  display.clearDisplay ();                     
  display.setRotation (2); 
  display.setTextSize (1);
  
  display.setTextColor (WHITE, BLACK);
  display.setCursor (5,0);
  display.print (" Room Status ");


  display.setTextColor (BLACK);
  display.setCursor (7,10);
  display.print ("Temp:");
  display.print(t);
  display.print ((char)167);
  display.print ("C");

  display.setCursor (7,20);
  display.print ("R.Hum:");
  display.print(h);
  display.print ("%");

  display.setCursor (7,30);
  display.print ("CO2:");
  display.print(c);
  display.print ("PPM");

  if(c >= 299 && c < 1100) {
    Serial.println("Good air quality.");
    display.setCursor (7,40);
    display.print ("AirQ:Good!");
  }
  else if(c >=1100 && c < 2000) {
    Serial.println("Aeration needed.");
    display.setCursor (7,40);
    display.print ("AirQ:Aerate!");
  }
  else if(c > 2000 && c < 5000) {
    Serial.println("Dangerous Air!");
    display.setCursor (7,40);
    display.print ("AirQ:Danger!");
  }
  else {
    Serial.println("Sensor fault");
    display.setCursor (7,40);
    display.print ("Sensor Fault!");
  }

  display.display ();  


  delay(10000); 
  
  
}

void recvOneChar() {
  if (Serial.available() > 0) {
    receivedChar = Serial.read();
    newData = true;
    if(receivedChar == 99) {  //99 is "c" character in ASCII
      czr.calibrateFreshAir();
      Serial.println("Calibration command...");
      newData = false;
    }
    else {
      Serial.println("Please press c for calibration");
    }

  }
}
