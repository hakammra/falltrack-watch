# FALL TRACK – Fall Detection & Heart Rate Monitoring Watch

## Overview

FALL TRACK is a wearable safety device designed to detect falls and monitor heart rate in real time. The device is intended for elderly individuals or people with medical conditions where falls or abnormal heart rate could be dangerous.

This project was developed as a personal engineering project during my first year studying Electronic Engineering.

The device continuously monitors motion and pulse rate. If a fall is detected, it checks the user’s heart rate and sends an alert to caregivers.

---

## Features

* Fall detection using MPU6050 accelerometer and gyroscope
* Heart rate monitoring using MAX3012 sensor
* SMS alert system using SIM800L GSM module
* Low-power wearable design powered by a 3.7V LiPo battery
* Push-button interaction for device control
* WiFi alert capability for cloud notifications
* Compact watch/bracelet style form factor

---

## Hardware Used

* ESP8266 (ESP-12F)
* MPU6050 IMU sensor
* MAX3012 Pulse sensor
* SIM800L GSM module
* 3.7V LiPo battery
* Push button
* Buzzer
* OLED display

---

## How It Works

1. The MPU6050 continuously monitors acceleration and orientation.
2. When a fall pattern is detected, the system verifies the user’s pulse.
3. If the pulse is abnormal or no response is detected, the system triggers an alert.
4. An SMS alert is sent to predefined contacts using the GSM module.

---

## Applications

* Elderly fall detection
* Remote health monitoring
* Personal safety wearables
* Medical alert systems

---

## Future Improvements

* GPS location tracking
* Mobile app integration
* Machine learning based fall detection
* Smaller custom PCB design
* Improved battery life

---

## Author

Abdul Hakam
First Year Electronic Engineering Student
