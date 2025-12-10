# 🚗 Smart Parking System (Autonomous & IoT-Enabled)
<p align="center">
  <img width="300" height="400" alt="IMG_3098" src="https://github.com/user-attachments/assets/53552802-7ce1-4d70-b8bb-18aa9888b671" />
</p>

**A "Programming for Artificial Intelligence" Final Project**

## 📖 Overview
The **Smart Parking System** is a mini Smart City scenario designed to solve the parking headache using V2I (Vehicle-to-Infrastructure) communication. 

Instead of driving blindly looking for a spot, our autonomous car "talks" to the parking lot via Bluetooth to find an open space and navigates there automatically. The system also manages the lot itself, tracking occupancy and flagging cars that overstay their time limit.

## ✨ Key Features

### 1. The Smart Infrastructure 🏢
* **Real-time Detection:** Ultrasonic sensors at every slot monitor occupancy.
* **Overstay Logic:** Tracks parking duration and triggers blinking LEDs (simulating a ticket) if a car stays too long.
* **Bluetooth Broadcasting:** Constantly broadcasts the status of available spots to nearby vehicles.

### 2. The Autonomous Car 🚙
* **Smart Navigation:** Receives data from the lot, randomly selects a free spot, and navigates to it without human intervention.
* **Obstacle Avoidance:** Safety logic that freezes the car instantly if an object or person blocks the path.
* **Bluetooth Integration:** ESP32-based communication with the central parking system.

## 🛠️ Tech Stack & Hardware
* **Microcontrollers:** ESP32 (x2 - Car & Infrastructure)
* **Sensors:** Ultrasonic Sensors (HC-SR04)
* **Communication:** Bluetooth Classic/BLE
* **Actuators:** DC Motors (Car movement), LEDs (Status indicators)
* **Language:** C++ (Arduino IDE)

## 👥 The Team
This project was built by:

* **Imtishal Haq**
* **Laiba Basharat**
* **Anamta Tariq**
* **Afaq Amin**
* **Affan Shahid**

## 🙏 Acknowledgements & Credits
A huge thank you to our instructors for their guidance, mentorship, and for pushing us to optimize our code and logic:

* **Miss Mariam Bint Imran**
* **Sir Ali Hamza**
---
*Built for the Programming for Artificial Intelligence Final Project.*
