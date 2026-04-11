-- MySQL Workbench Forward Engineering

SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

-- -----------------------------------------------------
-- Schema mydb
-- -----------------------------------------------------
-- -----------------------------------------------------
-- Schema hospital_iot
-- -----------------------------------------------------

-- -----------------------------------------------------
-- Schema hospital_iot
-- -----------------------------------------------------
CREATE SCHEMA IF NOT EXISTS `hospital_iot` DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci ;
USE `hospital_iot` ;

-- -----------------------------------------------------
-- Table `hospital_iot`.`Medico`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `hospital_iot`.`Medico` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `nome_completo` VARCHAR(100) NOT NULL,
  `crm` VARCHAR(20) NOT NULL,
  `especialidade` VARCHAR(80) NULL DEFAULT NULL,
  `email` VARCHAR(120) NOT NULL,
  `telefone` VARCHAR(20) NULL DEFAULT NULL,
  `criado_em` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `crm` (`crm` ASC) VISIBLE,
  UNIQUE INDEX `email` (`email` ASC) VISIBLE)
ENGINE = InnoDB
AUTO_INCREMENT = 6
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `hospital_iot`.`Leito`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `hospital_iot`.`Leito` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `numero_leito` VARCHAR(10) NOT NULL,
  `andar` TINYINT UNSIGNED NULL DEFAULT NULL,
  `ala` VARCHAR(40) NULL DEFAULT NULL,
  `ocupado` TINYINT(1) NULL DEFAULT '0',
  `criado_em` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `numero_leito` (`numero_leito` ASC) VISIBLE)
ENGINE = InnoDB
AUTO_INCREMENT = 3
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `hospital_iot`.`Paciente`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `hospital_iot`.`Paciente` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `nome_completo` VARCHAR(100) NOT NULL,
  `cpf` CHAR(11) NOT NULL,
  `data_nascimento` DATE NOT NULL,
  `sexo` ENUM('M', 'F', 'Outro') NULL DEFAULT NULL,
  `tipo_sanguineo` ENUM('A+', 'A-', 'B+', 'B-', 'AB+', 'AB-', 'O+', 'O-') NULL DEFAULT NULL,
  `alergias` TEXT NULL DEFAULT NULL,
  `telefone` VARCHAR(20) NULL DEFAULT NULL,
  `contato_emergencia` VARCHAR(100) NULL DEFAULT NULL,
  `telefone_emergencia` VARCHAR(20) NULL DEFAULT NULL,
  `leito_id` INT UNSIGNED NULL DEFAULT NULL,
  `medico_id` INT UNSIGNED NULL DEFAULT NULL,
  `internado_em` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  `alta_em` TIMESTAMP NULL DEFAULT NULL,
  `criado_em` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  UNIQUE INDEX `cpf` (`cpf` ASC) VISIBLE,
  INDEX `idx_paciente_leito` (`leito_id` ASC) VISIBLE,
  INDEX `idx_paciente_medico` (`medico_id` ASC) VISIBLE,
  CONSTRAINT `fk_paciente_leito`
    FOREIGN KEY (`leito_id`)
    REFERENCES `hospital_iot`.`Leito` (`id`)
    ON DELETE SET NULL
    ON UPDATE CASCADE,
  CONSTRAINT `fk_paciente_medico`
    FOREIGN KEY (`medico_id`)
    REFERENCES `hospital_iot`.`Medico` (`id`)
    ON DELETE SET NULL
    ON UPDATE CASCADE)
ENGINE = InnoDB
AUTO_INCREMENT = 6
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `hospital_iot`.`EvolucaoPaciente`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `hospital_iot`.`EvolucaoPaciente` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `paciente_id` INT UNSIGNED NOT NULL,
  `medico_id` INT UNSIGNED NULL DEFAULT NULL,
  `observacao` TEXT NOT NULL,
  `registrado_em` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  INDEX `fk_evolucao_medico` (`medico_id` ASC) VISIBLE,
  INDEX `idx_evolucao_paciente` (`paciente_id` ASC) VISIBLE,
  CONSTRAINT `fk_evolucao_medico`
    FOREIGN KEY (`medico_id`)
    REFERENCES `hospital_iot`.`Medico` (`id`)
    ON DELETE SET NULL
    ON UPDATE CASCADE,
  CONSTRAINT `fk_evolucao_paciente`
    FOREIGN KEY (`paciente_id`)
    REFERENCES `hospital_iot`.`Paciente` (`id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
AUTO_INCREMENT = 7
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


-- -----------------------------------------------------
-- Table `hospital_iot`.`LeituraSensor`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `hospital_iot`.`LeituraSensor` (
  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `paciente_id` INT UNSIGNED NOT NULL,
  `temperatura` DECIMAL(5,2) NULL DEFAULT NULL,
  `luminosidade` DECIMAL(8,2) NULL DEFAULT NULL,
  `velocidade_vento` DECIMAL(6,2) NULL DEFAULT NULL,
  `umidade` DECIMAL(5,2) NULL DEFAULT NULL,
  `registrado_em` TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`),
  INDEX `idx_sensor_paciente` (`paciente_id` ASC) VISIBLE,
  INDEX `idx_sensor_registrado` (`registrado_em` ASC) VISIBLE,
  CONSTRAINT `fk_sensor_paciente`
    FOREIGN KEY (`paciente_id`)
    REFERENCES `hospital_iot`.`Paciente` (`id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB
AUTO_INCREMENT = 792
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_0900_ai_ci;


SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
