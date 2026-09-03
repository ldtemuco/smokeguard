#pragma once
// =================================================================================================
#include <Arduino.h>
// =================================================================================================

/**
 * @struct AlertData
 * @brief Estructura para almacenar los datos de una alerta de SmokeGuard.
 *
 * Contiene la información necesaria para enviar una alerta a la API.
 */
struct AlertData
{
    /**
     * @brief Identificador único del dispositivo que genera la alerta.
     *
     * Ejemplo: "smokeguard-001".
     */
    String deviceId;


    /**
     * @brief Nombre del sensor que originó la alerta.
     *
     * Ejemplo: "SPS30", "SGP41" o "SHT45".
     */
    String sensor;


    /**
     * @brief Tipo de alerta detectada.
     *
     * Ejemplo: "PM25_HIGH", "VOC_HIGH" o "TEMPERATURE_HIGH".
     */
    String alertType;


    /**
     * @brief Valor de la medición que originó la alerta.
     */
    float value = 0.0f;


    /**
     * @brief Unidad de medida asociada al valor.
     *
     * Ejemplo: "ug/m3", "C" o "%".
     */
    String unit;


    /**
     * @brief Nivel de severidad de la alerta.
     *
     * Ejemplo: "warning" o "critical".
     */
    String level;
};