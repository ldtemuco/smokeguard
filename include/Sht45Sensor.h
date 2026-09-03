#pragma once
// =================================================================================================
#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSht4x.h>
// =================================================================================================
#include "SensorData.h"
/**
 * @class Sht45Sensor
 * @file Sht45Sensor.h
 * @brief Clase para manejar el sensor SHT45 de Sensirion.
**/
class Sht45Sensor 
{
    public:
         /**
         * @brief Inicializa el sensor SHT45.     
         * 
         * @param bus Bus I2C utilizado por el sensor.     
         * 
         * @retval true si el sensor fue detectado e inicializado.     
         * @retval false si el sensor no respondió.
         */
        bool begin(TwoWire& bus);

        /**
         * @brief Realiza una lectura del SHT45.     
         * 
         * Mide la temperatura y humedad del aire y almacena los resultados en la estructura 
         * SensorData.
         * 
         * @param data Estructura donde se almacenan las mediciones.
         *
         * @retval true Si la lectura fue realizada correctamente.
         * @retval false Si ocurrió un error.
         */    
        bool read(SensorData& data);


    private:
        /**
         * @brief Instancia del sensor SHT45.
         */
        SensirionI2cSht4x sensor;

        /**
         * @brief Indica si el sensor SHT45 fue detectado e inicializado.
         */
        bool available = false;
};