# 🌧️ ECODry — Smart Rain Cover System

Automated rain detection and cover control system for rubber drying operations.  
ESP32-powered IoT device with a real-time web dashboard and GSM SMS alerting.

---

## 🏗️ Architecture

```
┌─────────────┐     WiFi HTTP      ┌──────────────────┐
│   ESP32 +   │ ──────────────────▶│  XAMPP (Apache)   │
│ Rain Sensor │                    │    + MySQL DB     │
│ + Servo     │                    │                   │
│ + GSM/SIM   │                    │  /api/ endpoints  │
└─────────────┘                    │  /index.html      │
       │                           └──────────────────┘
       │ SMS (via GSM)                     ▲
       ▼                                   │ Browser
┌─────────────┐                    ┌──────────────────┐
│  Phone(s)   │                    │  Dashboard (JS)  │
│  SMS Alerts │                    │  Chart.js charts  │
└─────────────┘                    └──────────────────┘
```

## 📁 Project Structure

```
ECODry/
├── firmware/
│   └── ecodry_rain_detector.ino    # ESP32 Arduino firmware
├── database/
│   └── schema.sql                  # MySQL database schema
├── web/
│   ├── index.html                  # Dashboard UI
│   ├── css/
│   │   └── style.css               # Premium dark theme
│   ├── js/
│   │   └── app.js                  # Dashboard logic + charts
│   └── api/
│       ├── config.php              # Database connection
│       ├── insert_data.php         # ← ESP32 sends sensor data here
│       ├── update_health.php       # ← ESP32 sends health data here
│       ├── get_data.php            # → Dashboard reads sensor data
│       ├── get_health.php          # → Dashboard reads health status
│       └── get_stats.php           # → Dashboard reads statistics
└── README.md
```

## 🚀 Setup Guide

### 1. Database Setup

1. Open **phpMyAdmin** → `http://localhost/phpmyadmin`
2. Import `database/schema.sql` (or copy-paste and run it)
3. This creates the `ecodry_db` database with all tables

### 2. Web Server Deployment

1. Copy the entire `web/` folder to your XAMPP htdocs:
   ```
   C:\xampp\htdocs\rain_cover_system\
   ```
2. The structure should look like:
   ```
   C:\xampp\htdocs\rain_cover_system\
   ├── index.html
   ├── css/style.css
   ├── js/app.js
   └── api/
       ├── config.php
       ├── insert_data.php
       └── ...
   ```
3. Access the dashboard at: `http://localhost/rain_cover_system/`

### 3. ESP32 Firmware

1. Open `firmware/ecodry_rain_detector.ino` in **Arduino IDE**
2. Update the WiFi credentials and server IP if needed
3. Flash to your ESP32

### 4. Verify Everything

- **Test API**: `http://localhost/rain_cover_system/api/insert_data.php?rain=YES&cover=CLOSE&value=1200`
- **Dashboard**: `http://localhost/rain_cover_system/`
- **Health**: `http://localhost/rain_cover_system/api/get_health.php`

## ⚙️ ESP32 Pin Wiring

| Component    | ESP32 Pin     |
| ------------ | ------------- |
| Water Sensor | GPIO 34 (ADC) |
| Servo Motor  | GPIO 13       |
| GSM RX       | GPIO 16       |
| GSM TX       | GPIO 17       |

## 📡 API Endpoints

| Endpoint                                                        | Method | Description              |
| --------------------------------------------------------------- | ------ | ------------------------ |
| `/api/insert_data.php?rain=YES&cover=CLOSE&value=1200`          | GET    | Log sensor reading       |
| `/api/update_health.php?water_sensor=WORKING&servo=WORKING&...` | GET    | Update system health     |
| `/api/get_data.php?limit=50&range=24h`                          | GET    | Fetch sensor history     |
| `/api/get_health.php`                                           | GET    | Get current health       |
| `/api/get_stats.php`                                            | GET    | Get dashboard statistics |

## 📱 SMS Alert Recipients

Edit the `users[]` array in the firmware to add/remove SMS recipients:

```cpp
User users[MAX_USERS] = {
    {"User 1", "+639307662303", true},
    {"User 2", "+639XXXXXXXXX", true},
    {"User 3", "+639XXXXXXXXX", false},  // inactive
};
```

---

**Built for senior capstone / thesis project** · ESP32 + XAMPP + PHP + MySQL + Chart.js
