//* Final parking slots board with ultrasonic sensors and bluetooth slot info sender */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

#define NUM_SLOTS 4
const int trigPins[NUM_SLOTS] = {5, 18, 19, 23};
const int echoPins[NUM_SLOTS] = {4, 15, 2, 14};
const int ledPins[NUM_SLOTS]  = {13, 12, 27, 26};
LiquidCrystal_I2C lcd(0x27, 16, 2);

const long OCCUPIED_THRESHOLD_CM = 15;
const int SAMPLE_COUNT = 3;
const unsigned long LOOP_DELAY = 1000;
const unsigned long PARKING_LIMIT_MS = 10000;
 
struct SlotConfig {
  int id;
  float x;
  float y;
  String side;
};

SlotConfig slotConfigs[NUM_SLOTS] = {
  {1, 5.5, 6.0, "right"},
  {2, 5.5, 12.0, "right"},
  {3, 5.5, 6.0, "left"},
  {4, 5.5, 12.0, "left"}
};

// Track if overtime message has been printed
bool overtimeNotified[NUM_SLOTS] = {false, false, false, false};

long getDistance(int trig, int echo) {
  long total = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long dur = pulseIn(echo, HIGH, 30000);
    long dist = (dur == 0) ? 999 : (dur * 0.034 / 2.0);
    total += dist;
    delay(10);
  }
  return total / SAMPLE_COUNT;
}

void sendSlotData(bool occupied[]) {
  if (!SerialBT.hasClient()) {
    Serial.println(" No Bluetooth client connected");
    return;
  }
  
  String message = "SLOT_DATA:";
  
  for (int i = 0; i < NUM_SLOTS; i++) {
    bool isFree = !occupied[i];
    
    message += String(slotConfigs[i].id) + ",";
    message += String(slotConfigs[i].x, 1) + ",";
    message += String(slotConfigs[i].y, 1) + ",";
    message += slotConfigs[i].side + ",";
    message += (isFree ? "1" : "0");
    
    if (i < NUM_SLOTS - 1) message += ";";
  }
  
  SerialBT.println(message);
  
  Serial.print(" Sent via BT: ");
  Serial.println(message);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  if (!SerialBT.begin("ParkingDetector", true)) {
    Serial.println(" Bluetooth initialization failed!");
    return;
  }
  
  bool connected = SerialBT.connect("ParkingCar");

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");
  lcd.setCursor(0, 1);
  lcd.print(connected ? "BT: Connected" : "BT: Searching");
  delay(2000);
  lcd.clear();
  
  for (int i = 0; i < NUM_SLOTS; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {

  static bool wasConnected = false;
  bool isConnected = SerialBT.hasClient();

  int freeSlots = 0;
  bool occupied[NUM_SLOTS];
  static unsigned long slotOccupiedTime[NUM_SLOTS] = {0};

  for (int i = 0; i < NUM_SLOTS; i++) {

    long dist = getDistance(trigPins[i], echoPins[i]);

    if (dist <= OCCUPIED_THRESHOLD_CM) {

      occupied[i] = true;

      if (slotOccupiedTime[i] == 0) {
        slotOccupiedTime[i] = millis();
      }

      unsigned long parkedTime = millis() - slotOccupiedTime[i];

      if (parkedTime <= PARKING_LIMIT_MS) {
        digitalWrite(ledPins[i], HIGH);

        // Reset overtime notice when car leaves or is still within time
        overtimeNotified[i] = false;

      } else {
        // LED blinking (overtime)
        digitalWrite(ledPins[i], (millis() / 300) % 2);

        //Print ticket message ONCE
        if (!overtimeNotified[i]) {
          Serial.print(" Ticket issued! Car in Slot ");
          Serial.print(i + 1);
          Serial.println(" exceeded allowed parking time.  Challan: $10");
          overtimeNotified[i] = true;
        }
      }

    } else {
      occupied[i] = false;
      freeSlots++;
      slotOccupiedTime[i] = 0;
      overtimeNotified[i] = false;
      digitalWrite(ledPins[i], LOW);
    }
  }

  bool anyOvertime = false;
  for (int i = 0; i < NUM_SLOTS; i++) {
    if (slotOccupiedTime[i] != 0 &&
        millis() - slotOccupiedTime[i] > PARKING_LIMIT_MS) {
      anyOvertime = true;
      break;
    }
  }

  if (!anyOvertime) {
    lcd.setCursor(0, 0);
    lcd.print("Free: ");
    lcd.print(freeSlots);
    lcd.print(" BT:");
    lcd.print(isConnected ? "OK " : "XX ");

    lcd.setCursor(0, 1);
    for (int i = 0; i < NUM_SLOTS; i++) {
      lcd.print(occupied[i] ? "X" : "O");
      lcd.print(" ");
    }
  }

  sendSlotData(occupied);

  delay(LOOP_DELAY);
}
