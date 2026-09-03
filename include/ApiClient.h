#pragma once
// =================================================================================================
#include <Arduino.h>
// =================================================================================================
#include "AlertData.h"

/**
 * @class ApiClient
 * @file ApiClient.h
 * @brief Clase para manejar la comunicación con la API de SmokeGuard.
 *
 * Permite enviar alertas generadas por el dispositivo al servidor remoto mediante HTTPS.
 */
class ApiClient
{
    public:

        /**
         * @brief Envía una alerta a la API de SmokeGuard.
         *
         * Valida los datos de la alerta, los convierte al formato JSON esperado por la API
         * y realiza una solicitud HTTP POST mediante una conexión HTTPS segura.
         *
         * @param alert Datos de la alerta que se desea enviar.
         *
         * @retval true Si la alerta fue enviada correctamente y el servidor respondió
         * con un código HTTP exitoso.
         * @retval false Si los datos son inválidos o ocurrió un error durante la comunicación.
         */
        bool sendAlert(const AlertData& alert);


    private:

        /**
         * @brief Comprueba que los datos mínimos de una alerta sean válidos.
         *
         * @param alert Datos de la alerta.
         *
         * @retval true Si los datos son válidos.
         * @retval false Si falta información obligatoria o el valor numérico no es válido.
         */
        bool validateAlert(const AlertData& alert) const;


        /**
         * @brief Convierte una alerta al formato JSON utilizado por la API.
         *
         * @param alert Datos de la alerta.
         *
         * @return Cadena JSON lista para ser enviada al servidor.
         */
        String createAlertJson(const AlertData& alert) const;


        /**
         * @brief Escapa una cadena para que pueda utilizarse de forma segura dentro de JSON.
         *
         * @param value Cadena original.
         *
         * @return Cadena escapada.
         */
        String escapeJsonString(const String& value) const;
};