#pragma once
// =================================================================================================
#include <Arduino.h>
// =================================================================================================


/**
 * @struct WiFiNetwork
 * @brief Representa una red Wi-Fi almacenada por SmokeGuard.
 */
struct WiFiNetwork
{
    /**
     * @brief Nombre de la red Wi-Fi.
     */
    String ssid;


    /**
     * @brief Contraseña de la red Wi-Fi.
     *
     * Puede estar vacía para redes abiertas.
     */
    String password;
};