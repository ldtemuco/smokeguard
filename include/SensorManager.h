#pragma once
// =================================================================================================
#include <Arduino.h>
#include <Wire.h>
// =================================================================================================
#include "SensorData.h"
#include "Sht45Sensor.h"
#include "Sgp41Sensor.h"
#include "Sps30Sensor.h"
// =================================================================================================

/**
 * @class SensorManager
 * @file SensorManager.h
 * @brief Clase para administrar los sensores de SmokeGuard.
 *
 * Coordina la inicialización y lectura de los sensores SHT45, SGP41 y SPS30.
 *
 * El SHT45 y el SGP41 utilizan el bus I2C principal, mientras que el SPS30 utiliza un segundo
 * bus I2C.
 *
 * Las mediciones obtenidas se almacenan en una estructura SensorData compartida.
 */
class SensorManager
{
    public:

        /**
         * @brief Inicializa los buses I2C y los sensores de SmokeGuard.
         *
         * @retval true Si todos los sensores fueron inicializados correctamente.
         * @retval false Si uno o más sensores no pudieron ser inicializados.
         */
        bool begin();


        /**
         * @brief Actualiza las mediciones de los sensores.
         *
         * Las lecturas se realizan utilizando el intervalo configurado para los sensores.
         */
        void update();


        /**
         * @brief Obtiene los últimos datos registrados por los sensores.
         *
         * @return Referencia constante a la estructura SensorData.
         */
        const SensorData& getData() const;


    private:

        /**
         * @brief Instancia del sensor SHT45.
         */
        Sht45Sensor sht45;

        /**
         * @brief Instancia del sensor SGP41.
         */
        Sgp41Sensor sgp41;

        /**
         * @brief Instancia del sensor SPS30.
         */
        Sps30Sensor sps30;

        /**
         * @brief Almacena las últimas mediciones obtenidas de los sensores.
         */
        SensorData data;

        /**
         * @brief Momento en que se realizó la última actualización de los sensores.
         */
        uint32_t lastUpdateMillis = 0;
};