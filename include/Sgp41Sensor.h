#pragma once
// =================================================================================================
#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2CSgp41.h>
#include <VOCGasIndexAlgorithm.h>
#include <NOxGasIndexAlgorithm.h>
// =================================================================================================
#include "SensorData.h"
/**
 * @class Sgp41Sensor 
 * @file Sgp41Sensor.h
 * @brief Clase para manejar el sensor SGP41 de Sensirion.
**/
class Sgp41Sensor 
{
    public:

        /**
         * @brief Inicializa el sensor SGP41.     
         * 
         * @param bus Bus I2C utilizado por el sensor.     
         * 
         * @retval true si el sensor fue detectado e inicializado.     
         * @retval false si el sensor no respondió.
         */
        bool begin(TwoWire& bus);


        /**
         * @brief Realiza una lectura del SGP41.     
         * 
         * Mide la cantidad de compuestos orgánicos volátiles (VOC) y óxidos de nitrógeno (NOx) 
         * en el aire y almacena los resultados en la estructura SensorData.
         * 
         * Utiliza los datos de temperatura y humedad del SHT45 para compensar las mediciones 
         * cuando estos datos están disponibles.
         *
         * @param data Estructura donde se almacenan las mediciones.
         *
         * @retval true Si la lectura fue realizada correctamente.
         * @retval false Si ocurrió un error.
         */
        bool read(SensorData& data);


    private:

        /**
         * @brief Instancia del sensor SGP41.
         */
        SensirionI2CSgp41 sensor;

        /**
         * @brief Algoritmo para calcular el índice VOC.
         */
        VOCGasIndexAlgorithm vocAlgorithm;

        /**
         * @brief Algoritmo para calcular el índice NOx.
         */
        NOxGasIndexAlgorithm noxAlgorithm;

        /**
         * @brief Indica si el sensor SGP41 fue detectado e inicializado.
         */
        bool available = false;

        /**
         * @brief Segundos restantes del período de acondicionamiento NOx.
         */
        uint8_t conditioningSeconds = 10;

        /**
         * @brief Convierte humedad relativa a ticks para el SGP41.
         *
         * @param humidity Humedad relativa en porcentaje.
         *
         * @return Humedad convertida al formato utilizado por el SGP41.
         */
        uint16_t humidityToTicks(float humidity);


        /**
         * @brief Convierte temperatura a ticks para el SGP41.
         *
         * @param temperature Temperatura en grados Celsius.
         *
         * @return Temperatura convertida al formato utilizado por el SGP41.
         */
        uint16_t temperatureToTicks(float temperature);
};