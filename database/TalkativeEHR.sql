-- ============================================================
--  Hospital IoT Database Schema
--  Project: ESP32 + Alexa + EHR Frontend
--  Database: MySQL
-- ============================================================

CREATE DATABASE IF NOT EXISTS hospital_iot;
USE hospital_iot;

-- ============================================================
-- TABLE: Doctor
-- ============================================================
CREATE TABLE Doctor (
    id          INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    full_name   VARCHAR(100)        NOT NULL,
    crm         VARCHAR(20)         NOT NULL UNIQUE,   -- Brazilian medical license
    specialty   VARCHAR(80),
    email       VARCHAR(120)        NOT NULL UNIQUE,
    phone       VARCHAR(20),
    created_at  TIMESTAMP           DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- TABLE: Room (Leito)
-- ============================================================
CREATE TABLE Room (
    id          INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    room_number VARCHAR(10)         NOT NULL UNIQUE,
    floor       TINYINT UNSIGNED,
    wing        VARCHAR(40),                           -- e.g. "UTI", "Pediatria"
    is_occupied BOOLEAN             DEFAULT FALSE,
    created_at  TIMESTAMP           DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- TABLE: Patient (Paciente)
-- ============================================================
CREATE TABLE Patient (
    id              INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    full_name       VARCHAR(100)    NOT NULL,
    cpf             CHAR(11)        NOT NULL UNIQUE,   -- Brazilian national ID
    birth_date      DATE            NOT NULL,
    gender          ENUM('M','F','Other'),
    blood_type      ENUM('A+','A-','B+','B-','AB+','AB-','O+','O-'),
    allergies       TEXT,
    phone           VARCHAR(20),
    emergency_contact VARCHAR(100),
    emergency_phone VARCHAR(20),
    -- Relationships
    room_id         INT UNSIGNED,
    doctor_id       INT UNSIGNED,
    -- Timestamps
    admitted_at     TIMESTAMP       DEFAULT CURRENT_TIMESTAMP,
    discharged_at   TIMESTAMP       NULL,
    created_at      TIMESTAMP       DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT fk_patient_room
        FOREIGN KEY (room_id)   REFERENCES Room(id)   ON UPDATE CASCADE ON DELETE SET NULL,
    CONSTRAINT fk_patient_doctor
        FOREIGN KEY (doctor_id) REFERENCES Doctor(id) ON UPDATE CASCADE ON DELETE SET NULL
);

-- ============================================================
-- TABLE: Patient Development (Evolução do Paciente)
-- One patient → many development entries (1:N)
-- ============================================================
CREATE TABLE PatientDevelopment (
    id              INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    patient_id      INT UNSIGNED    NOT NULL,
    doctor_id       INT UNSIGNED,                      -- who wrote the note
    note            TEXT            NOT NULL,          -- clinical observation / evolution
    recorded_at     TIMESTAMP       DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT fk_dev_patient
        FOREIGN KEY (patient_id) REFERENCES Patient(id) ON UPDATE CASCADE ON DELETE CASCADE,
    CONSTRAINT fk_dev_doctor
        FOREIGN KEY (doctor_id)  REFERENCES Doctor(id)  ON UPDATE CASCADE ON DELETE SET NULL
);

-- ============================================================
-- TABLE: Sensor Reading (ESP32 data — linked to Patient)
-- Stores environment data captured in the patient's room
-- ============================================================
CREATE TABLE SensorReading (
    id              INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    patient_id      INT UNSIGNED    NOT NULL,
    temperature     DECIMAL(5,2),                      -- °C
    luminosity      DECIMAL(8,2),                      -- lux
    wind_speed      DECIMAL(6,2),                      -- m/s  (or air flow)
    humidity        DECIMAL(5,2),                      -- % (bonus — cheap to add)
    recorded_at     TIMESTAMP       DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT fk_sensor_patient
        FOREIGN KEY (patient_id) REFERENCES Patient(id) ON UPDATE CASCADE ON DELETE CASCADE
);

-- ============================================================
-- INDEXES (performance)
-- ============================================================
CREATE INDEX idx_patient_room       ON Patient(room_id);
CREATE INDEX idx_patient_doctor     ON Patient(doctor_id);
CREATE INDEX idx_dev_patient        ON PatientDevelopment(patient_id);
CREATE INDEX idx_sensor_patient     ON SensorReading(patient_id);
CREATE INDEX idx_sensor_recorded    ON SensorReading(recorded_at);