#pragma once
// =================================================================================================
#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSps30.h>
// =================================================================================================
#include "SensorData.h"
/**
 * @class Sps30Sensor 
 * @file Sps30Sensor.h
 * @brief Clase para manejar el sensor SPS30 de Sensirion.
**/
class Sps30Sensor 
{
    public:
        /**
         * @brief Inicializa el sensor SPS30.     
         * 
         * @param bus Bus I2C utilizado por el sensor.     
         * 
         * @retval true si el sensor fue detectado e inicializado.     
         * @retval false si el sensor no respondió.
         */
        bool begin(TwoWire& bus);

        /**
         * @brief Realiza una lectura del SPS30.     
         * 
         * Mide el material particulado del aire y almacena los resultados en la estructura 
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
         * @brief Instancia del sensor SPS30.
         */
        SensirionI2cSps30 sensor;

        /**
         * @brief Indica si el sensor SPS30 fue detectado e inicializado.
         */
        bool available = false;
};