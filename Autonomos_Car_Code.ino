#include <Arduino.h>
#include <BluetoothSerial.h>

BluetoothSerial SerialBT;

#define ENA 2
#define IN1 4
#define IN2 5
#define ENB 21
#define IN3 18
#define IN4 19

#define TRIG 32
#define ECHO 33
#define LED 25 

const int OBSTACLE_DISTANCE = 15;
bool obstacleCheckEnabled = true;
int currentSelectedSlot = -1;  

// Constants
const int SPEED = 180;
const float MS_PER_INCH = 200.0;
const int ROTATE_90_MS = 2065;

//Slot Data 
struct Slot {
  int id;
  float x;
  float y;
  String side;
  bool isFree;
};

Slot slots[4] = {
  {1, 5.5, 6.0, "right", false},
  {2, 5.5, 12.0, "right", false},
  {3, 5.5, 6.0, "left", false},
  {4, 5.5, 12.0, "left", false}
};

bool parkingComplete = false;

//Distance Measurement
long getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 25000);
  long distance = duration * 0.034 / 2;
  return distance == 0 ? 400 : distance;
}

// LED Control
void ledBlink() {
  digitalWrite(LED, HIGH);
  delay(200);
  digitalWrite(LED, LOW);
  delay(200);
}

void waitForObstacleClear() {
  Serial.println("\nOBSTACLE DETECTED! WAITING FOR CLEARANCE...");
  
  while (true) {
    long d = getDistance();
    
    if (d >= OBSTACLE_DISTANCE) {
      digitalWrite(LED, LOW); 
      Serial.println("Obstacle cleared! Resuming...\n");
      delay(500);
      break;
    }
    
    ledBlink();
    
    Serial.print("  Obstacle still present: ");
    Serial.print(d);
    Serial.println(" cm");
  }
}

// Motor Control Functions
void stopMotors() {
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  Serial.println("  [STOP]");
}

// Forward declarations
void rotateLeft90();
void rotateRight90();

void moveForward(float inches) {
  if (inches < 0.1) return;

  float remainingDistance = inches;
  unsigned long totalTime = inches * MS_PER_INCH;
  unsigned long startTime = millis();

  Serial.print("  Moving FORWARD ");
  Serial.print(inches);
  Serial.println(" inches");

  while (remainingDistance > 0.1) {
    unsigned long elapsed = millis() - startTime;
    float traveledDistance = elapsed / MS_PER_INCH;
    remainingDistance = inches - traveledDistance;

    long d = getDistance();
    Serial.print("  Distance: ");
    Serial.print(d);
    Serial.print(" cm | Remaining: ");
    Serial.print(remainingDistance);
    Serial.println(" inches");

    // OBSTACLE DETECTED
    if (obstacleCheckEnabled && d > 0 && d < OBSTACLE_DISTANCE) {
      stopMotors();
      delay(300);
      
      waitForObstacleClear();
      
      startTime = millis();
      inches = remainingDistance;
      totalTime = remainingDistance * MS_PER_INCH;
    }

 
    if (millis() - startTime >= totalTime) {
      break;
    }


    digitalWrite(IN1, HIGH); 
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); 
    digitalWrite(IN4, LOW);
    ledcWrite(0, SPEED);
    ledcWrite(1, SPEED);

    delay(50);
  }

  stopMotors();
  delay(300);
  Serial.println("    Forward movement complete");
}

void rotateLeft90() {
  Serial.print("  Rotating LEFT 90° (");
  Serial.print(ROTATE_90_MS/1000.0);
  Serial.println(" sec)");
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  ledcWrite(0, SPEED);
  
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  ledcWrite(1, SPEED);
  
  delay(ROTATE_90_MS);
  stopMotors();
  delay(500);
}

void rotateRight90() {
  Serial.print("  Rotating RIGHT 90° (");
  Serial.print(ROTATE_90_MS/1000.0);
  Serial.println(" sec)");
  
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  ledcWrite(0, 160);  // Slower left motor
  
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  ledcWrite(1, 180);  // Normal right motor
  
  delay(ROTATE_90_MS);
  stopMotors();
  delay(500);
}

// Bluetooth Slot Update
bool updateSlotsFromBluetooth(String data) {
  int startIndex = data.indexOf("SLOT_DATA:") + 10;
  if (startIndex < 10) {
    return false;
  }
  
  String slotData = data.substring(startIndex);
  slotData.trim();
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║       Parsing Slot Data                  ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  int slotIndex = 0;
  int currentPos = 0;
  
  while (slotIndex < 4 && currentPos < slotData.length()) {
    int nextSemicolon = slotData.indexOf(';', currentPos);
    if (nextSemicolon == -1) {
      nextSemicolon = slotData.length();
    }
    
    String slotInfo = slotData.substring(currentPos, nextSemicolon);
    
    int comma1 = slotInfo.indexOf(',');
    int comma2 = slotInfo.indexOf(',', comma1 + 1);
    int comma3 = slotInfo.indexOf(',', comma2 + 1);
    int comma4 = slotInfo.indexOf(',', comma3 + 1);
    
    if (comma1 > 0 && comma2 > 0 && comma3 > 0 && comma4 > 0) {
      slots[slotIndex].id = slotInfo.substring(0, comma1).toInt();
      slots[slotIndex].x = slotInfo.substring(comma1 + 1, comma2).toFloat();
      slots[slotIndex].y = slotInfo.substring(comma2 + 1, comma3).toFloat();
      slots[slotIndex].side = slotInfo.substring(comma3 + 1, comma4);
      String freeStatus = slotInfo.substring(comma4 + 1);
      freeStatus.trim();
      slots[slotIndex].isFree = (freeStatus == "1");
      
      Serial.print("  Slot ");
      Serial.print(slots[slotIndex].id);
      Serial.print(": (");
      Serial.print(slots[slotIndex].x, 1);
      Serial.print(", ");
      Serial.print(slots[slotIndex].y, 1);
      Serial.print(") ");
      Serial.print(slots[slotIndex].side);
      Serial.print(" - ");
      Serial.println(slots[slotIndex].isFree ? "FREE " : "OCCUPIED ");
      
      slotIndex++;
    }
    
    currentPos = nextSemicolon + 1;
  }
  
  Serial.println();
  return (slotIndex == 4);
}

//Select Random FREE Slot
int selectRandomSlot() {
  int freeSlots[4];
  int freeCount = 0;
  
  for (int i = 0; i < 4; i++) {
    if (slots[i].isFree) {
      freeSlots[freeCount] = i;
      freeCount++;
    }
  }
  
  if (freeCount == 0) return -1;
  
  int randomIndex = random(0, freeCount);
  return freeSlots[randomIndex];
}

//Movement to Slot
void moveToSlot(int slotIndex) {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.print("║   Moving to Slot ");
  Serial.print(slots[slotIndex].id);
  Serial.println("                    ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  currentSelectedSlot = slotIndex;
  float targetX = slots[slotIndex].x;
  float targetY = slots[slotIndex].y;
  
  Serial.print("Target position: (");
  Serial.print(targetX);
  Serial.print(", ");
  Serial.print(targetY);
  Serial.println(")");
  
  Serial.println("\n┌─ Step 1: Move Forward to Y position ──┐");
  moveForward(targetY);
  
  Serial.println("\n┌─ Step 2: Rotate toward slot ──────────┐");
  if (slots[slotIndex].side == "right") {
    rotateRight90();
  } else {
    rotateLeft90();
  }
  
  Serial.println("\n┌─ Step 3: Enter Slot ──────────────────┐");
  obstacleCheckEnabled = false;
  Serial.println("Obstacle detection DISABLED for final parking");

  float distanceToSlot = abs(targetX);
  Serial.print("Distance to slot: ");
  Serial.print(distanceToSlot);
  Serial.println(" inches");
  
  moveForward(distanceToSlot);

  obstacleCheckEnabled = true;
  
  Serial.println("\n PARKING COMPLETE!");
  Serial.print("Final position: (");
  Serial.print(slots[slotIndex].x);
  Serial.print(", ");
  Serial.print(slots[slotIndex].y);
  Serial.println(")");
}

// ------------ SETUP ------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   Autonomous Parking Car             ║");
  Serial.println("║     [WITH LED ALERT SYSTEM]            ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  if (!SerialBT.begin("ParkingCar", false)) {
    Serial.println(" Bluetooth initialization failed!");
    return;
  }
  
  Serial.println("✓ Bluetooth initialized: ParkingCar (SLAVE)");
  Serial.println("✓ Waiting for detector to connect...\n");
  
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);  

  pinMode(IN1, OUTPUT); 
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); 
  pinMode(IN4, OUTPUT);
  
  ledcSetup(0, 5000, 8);
  ledcAttachPin(ENA, 0);
  ledcSetup(1, 5000, 8);
  ledcAttachPin(ENB, 1);
  
  randomSeed(analogRead(34) + millis());
  
  Serial.println("Waiting for slot data (30 sec timeout)...\n");
  
  unsigned long startTime = millis();
  bool dataReceived = false;
  int receiveCount = 0;
  
  while (millis() - startTime < 30000 && !dataReceived) {
    if (SerialBT.available()) {
      String received = SerialBT.readStringUntil('\n');
      received.trim();
      
      receiveCount++;
      Serial.print(" [");
      Serial.print(receiveCount);
      Serial.print("] Received: ");
      Serial.println(received);
      
      if (received.indexOf("SLOT_DATA:") >= 0) {
        if (updateSlotsFromBluetooth(received)) {
          dataReceived = true;
          Serial.println("✓✓✓ Valid slot data received! ✓✓✓\n");
        }
      }
    }
    delay(100);
  }
  
  if (!dataReceived) {
    Serial.println("\n ERROR: No valid data received!");
    Serial.println("   Check detector Bluetooth connection\n");
    return;
  }
  
  delay(2000);
  
  int selected = selectRandomSlot();
  
  if (selected == -1) {
    Serial.println(" No free slots available!");
    return;
  }
  
  Serial.print("\n Selected Slot ");
  Serial.println(slots[selected].id);
  
  delay(2000);
  
  moveToSlot(selected);
  parkingComplete = true;
  
  slots[selected].isFree = false;
}

void loop() {
  if (!parkingComplete && SerialBT.available()) {
    String received = SerialBT.readStringUntil('\n');
    received.trim();
    
    if (received.indexOf("SLOT_DATA:") >= 0) {
      Serial.println("\n New data received:");
      updateSlotsFromBluetooth(received);
    }
  }
  
  delay(1000);
}
