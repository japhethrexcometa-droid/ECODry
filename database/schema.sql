-- =============================================
-- ECODry Rain Cover System - Database Schema
-- =============================================
-- Run this in phpMyAdmin or MySQL CLI to set up

CREATE DATABASE IF NOT EXISTS ecodry_db
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE ecodry_db;

-- =============================================
-- Sensor readings table
-- =============================================
CREATE TABLE IF NOT EXISTS sensor_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    rain_status ENUM('YES', 'NO') NOT NULL,
    cover_status ENUM('OPEN', 'CLOSE') NOT NULL,
    sensor_value INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_created (created_at),
    INDEX idx_rain (rain_status)
) ENGINE=InnoDB;

-- =============================================
-- System health table
-- =============================================
CREATE TABLE IF NOT EXISTS system_health (
    id INT AUTO_INCREMENT PRIMARY KEY,
    water_sensor VARCHAR(20) DEFAULT 'UNKNOWN',
    servo VARCHAR(20) DEFAULT 'UNKNOWN',
    sd_card VARCHAR(20) DEFAULT 'UNKNOWN',
    sd_space INT DEFAULT 0,
    gsm VARCHAR(20) DEFAULT 'UNKNOWN',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- Insert initial health row (we update this single row)
INSERT INTO system_health (water_sensor, servo, sd_card, sd_space, gsm)
VALUES ('UNKNOWN', 'UNKNOWN', 'UNKNOWN', 0, 'UNKNOWN');

-- =============================================
-- SMS log table
-- =============================================
CREATE TABLE IF NOT EXISTS sms_log (
    id INT AUTO_INCREMENT PRIMARY KEY,
    recipient_name VARCHAR(100),
    phone_number VARCHAR(20) NOT NULL,
    message TEXT NOT NULL,
    status ENUM('SENT', 'FAILED') DEFAULT 'SENT',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_created (created_at)
) ENGINE=InnoDB;

-- =============================================
-- Alert events table (tracks state changes)
-- =============================================
CREATE TABLE IF NOT EXISTS alert_events (
    id INT AUTO_INCREMENT PRIMARY KEY,
    event_type ENUM('RAIN_START', 'RAIN_STOP') NOT NULL,
    sensor_value INT NOT NULL,
    cover_action ENUM('OPEN', 'CLOSE') NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_created (created_at),
    INDEX idx_type (event_type)
) ENGINE=InnoDB;
