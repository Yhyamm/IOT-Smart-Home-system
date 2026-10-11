// Configuration of the Connection between the Template and the esp32
#define BLYNK_TEMPLATE_ID "TMPL64tlPi-tb"
#define BLYNK_TEMPLATE_NAME "lcd replacement"
#define BLYNK_AUTH_TOKEN "dEvwRoo053X3ExczsOc-pcZ_fxolTUBd"


// Required libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <Keypad.h>


// WiFi Credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Galaxy";
char pass[] = "tuuz0000";


// Pin Definitions (final safe map)
#define DHTPIN 27
#define DHTTYPE DHT11
#define PIR_PIN 33
#define MQ2_PIN 32
#define LDR_PIN 35
#define SERVO_PIN 14
#define BUZZER_PIN 15
#define RED_LED 4        // moved off strapping pin 15
#define GREEN_LED1 26
#define YELLOW_LED2 25


DHT dht(DHTPIN, DHTTYPE);
Servo myServo;
BlynkTimer timer;


// Keypad Setup (safe pins only)
const byte ROWS = 4;
const byte COLS = 4;


char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {21, 22, 23, 18};  
byte colPins[COLS] = {16, 17, 19, 13};   // replaced invalid GPIO20 with GPIO13
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


// Variables
String password = "1111";
String input_password = "";
int wrong_attempts = 0;
bool gasTriggered = false;
bool doorOpen = false;
unsigned long doorCloseAt = 0;


// Gas alarm cooldown
const int GAS_THRESH = 1500;
const unsigned long EVENT_COOLDOWN = 60000; // 1 min
unsigned long lastEventAt = 0;


// LDR hysteresis
const int ON_THRESH = 900;
const int OFF_THRESH = 1100;
bool lightsOn = false;


// Function to send sensor data to Blynk App
void sendSensorData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int gasValue = analogRead(MQ2_PIN);
  bool motion = digitalRead(PIR_PIN);


  Blynk.virtualWrite(V1, temp);
  Blynk.virtualWrite(V2, hum);
  Blynk.virtualWrite(V3, gasValue);
 
  if(motion){
  Blynk.virtualWrite(V5,"MOTION!");
  }
  else
  Blynk.virtualWrite(V5,"Secure");
 
  // Gas Alarm Flag + cooldown
  bool above = gasValue > GAS_THRESH;
  if (above && !gasTriggered) {
      gasTriggered = true;
      Blynk.virtualWrite(V1, gasValue);
      Blynk.logEvent("smoke_alert", "Warning! Smoke detected!");
    }
  else if (!above) {
    gasTriggered = false;
  }


}
// Non-blocking door control
void openDoorFor(unsigned long ms) {
  myServo.write(90);
  doorOpen = true;
  doorCloseAt = millis() + ms;
}
// Blynk Button to Control Door (V4)
BLYNK_WRITE(V4) {
  int value = param.asInt();
  if (value == 1) {
    openDoorFor(3000);
  } else {
    myServo.write(0);
    doorOpen = false;
  }
}




void setup() {
  Serial.begin(115200);
 
  Blynk.begin(auth, ssid, pass);
 
  dht.begin();
  myServo.attach(SERVO_PIN);
  myServo.write(0);
 
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED1, OUTPUT);
  pinMode(YELLOW_LED2, OUTPUT);
 
  digitalWrite(GREEN_LED1, HIGH);
  digitalWrite(YELLOW_LED2, HIGH);
  digitalWrite(BUZZER_PIN, LOW);


  timer.setInterval(2000L, sendSensorData);
}


void loop() {
  Blynk.run();
  timer.run();
 
  // Door auto-close
  if (doorOpen && millis() >= doorCloseAt) {
    myServo.write(0);
    doorOpen = false;
  }


  // Buzzer logic
  if (gasTriggered || wrong_attempts >= 2) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(3000);
    digitalWrite(BUZZER_PIN, LOW);
    wrong_attempts = 0;
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }


  // Keypad logic
  // Keypad logic
char key = keypad.getKey();
if (key) {
  if (key == '#') {
    if (input_password == password) {
      openDoorFor(3000);
      input_password = "";
      wrong_attempts = 0;
      digitalWrite(RED_LED, LOW);
    } else {
      wrong_attempts++;
      input_password = "";
    }
  } else if (key == '*') {
    input_password = "";
  } else {
    input_password += key;
  }


  // Send current input to Blynk app
  Blynk.virtualWrite(V6, input_password);
}




  // LED feedback for wrong attempts
  if (wrong_attempts >= 2) {
    digitalWrite(RED_LED, HIGH);
    delay (3000);
    digitalWrite(RED_LED, LOW);


  } else {
    digitalWrite(RED_LED, LOW);
  }


  // Smart lighting with hysteresis
  int lightLevel = analogRead(LDR_PIN);
  if (!lightsOn && lightLevel < ON_THRESH) {
    lightsOn = true;
    digitalWrite(GREEN_LED1, LOW);
    digitalWrite(YELLOW_LED2, LOW);
  } else if (lightsOn && lightLevel > OFF_THRESH) {
    lightsOn = false;
    digitalWrite(GREEN_LED1, HIGH);
    digitalWrite(YELLOW_LED2, HIGH);
  }
}
