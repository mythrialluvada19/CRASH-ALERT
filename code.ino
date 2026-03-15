#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <AltSoftSerial.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <math.h>

const String EMERGENCY_PHONE = "+917989196925";

#define rxPin 2
#define txPin 3
SoftwareSerial sim800(rxPin, txPin);

#define BUZZER 5
#define BUTTON 6

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

float magnitude = 0;
int sensitivity = 50;
boolean impact_detected = false;
unsigned long time1;
unsigned long impact_time;
unsigned long alert_delay = 30000;

String latitude;
String longitude;

AltSoftSerial neogps;
TinyGPSPlus gps;

void setup() {
  Serial.begin(9600);

  Serial.println("SIM800A serial initialize");
  sim800.begin(9600);
  neogps.begin(9600);

  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  sim800.println("AT");
  delay(1000);
  sim800.println("ATE1");
  delay(1000);
  sim800.println("AT+CPIN?");
  delay(1000);
  sim800.println("AT+CMGF=1");
  delay(1000);
  sim800.println("AT+CNMI=1,1,0,0,0");
  delay(1000);

  if (!accel.begin()) {
    Serial.println("Couldn't find the ADXL345 sensor");
    while (1);
  }
  accel.setRange(ADXL345_RANGE_16_G);
  time1 = micros();
}

void loop() {
  if (micros() - time1 > 1999) {
    Impact();
  }

  if (digitalRead(BUTTON) == HIGH) {
    delay(50); // Debounce
    if (digitalRead(BUTTON) == LOW) {
      Serial.println("Button pressed!");
      button();
    }
  }
}
void parseData(String data) {
  data.trim(); // Clean up any extra spaces or newline characters

  if (data.startsWith("+CMTI")) {
    // Handle incoming SMS notification
    String msg_index = data.substring(data.indexOf(",") + 1);
    sim800.println("AT+CMGR=" + msg_index + "\r");
  } 
  else if (data.startsWith("+CMGR")) {
    // Handle reading SMS content
    if (data.indexOf(EMERGENCY_PHONE) > 0 && data.indexOf("get gps") > 0) {
       // Fetch GPS coordinates
      String location_data = "GPS Location Data\rhttp://maps.google.com/maps?q=:" + latitude + "," + longitude;
       // Send GPS location via SMS
    }
  }
}
void Impact() {
   static unsigned long buzzerStartTime = 0;

  sensors_event_t event;
  accel.getEvent(&event);

  float deltax = event.acceleration.x;
  float deltay = event.acceleration.y;
  float deltaz = event.acceleration.z;

  magnitude = sqrt(sq(deltax) + sq(deltay) + sq(deltaz));

  if (magnitude >= sensitivity && !impact_detected) {
    impact_detected = true;
    impact_time = millis();
    buzzerStartTime = millis();  // Start buzzer timer

    digitalWrite(BUZZER, HIGH);
    Serial.println("Impact Detected! Magnitude: " + String(magnitude));
  }

  // Turn off the buzzer after 5 seconds
  if (impact_detected && millis() - buzzerStartTime >= 5000) {
    digitalWrite(BUZZER, LOW);
    impact_detected = false;
    makeCall();
    sendSms();
  }
}

void button() {
  digitalWrite(BUZZER, LOW);
  delay(500); // Buzzer active for 5 seconds
  digitalWrite(BUZZER, LOW);
  makeCall();
  sendAlert();
  digitalWrite(BUZZER, LOW);
  impact_detected = false;
}


void sendAlert() {
  String alert_message = "Accident Alert!!\rhttp://maps.google.com/maps?q=loc:" + latitude + "," + longitude;
  sendSms();
}

void makeCall() {
  Serial.println("Calling...");

  sim800.println("ATD" + EMERGENCY_PHONE + ";");
  delay(20000); // Wait for 20 seconds for the call
  sim800.println("ATH");
  delay(100);
}

void sendSms() {
  sim800.println("AT+CMGS=\"" + EMERGENCY_PHONE + "\""); // 1)
  delay(2000);
  sim800.print("Accident Alert!!\rhttp://maps.google.com/maps?q=:" + latitude + "," + longitude); // 2) text content 
  sim800.print("Emergency!! please track +91xxxxxxxxxx ");
  delay(1000);
  sim800.write(26);
  Serial.println("SMS Sent Successfully.");
}
