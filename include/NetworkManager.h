#pragma once
// =================================================================================================
#include <Arduino.h>
#include <WiFi.h>
// =================================================================================================
#include "WiFiNetwork.h"
// =================================================================================================
/**
 * @class NetworkManager
 * @file NetworkManager.h
 * @brief Clase para manejar la conexión de red de SmokeGuard.
 *
 * Permite conectar el dispositivo a redes Wi-Fi almacenadas,
 * sincronizar el reloj mediante NTP, comprobar el estado de la
 * conexión y verificar la resolución DNS.
 *
 * SmokeGuard utiliza simultáneamente los modos AP y STA para mantener
 * disponible el panel de configuración mientras se conecta a una red externa.
 */
class NetworkManager
{
    public:

        /**
         * @brief Conecta el dispositivo a una red Wi-Fi.
         *
         * Mantiene habilitado el modo AP mientras intenta establecer
         * la conexión STA.
         *
         * @param ssid Nombre de la red Wi-Fi.
         * @param password Contraseña de la red Wi-Fi.
         *
         * @retval true Si la conexión fue realizada correctamente.
         * @retval false Si no fue posible establecer la conexión.
         */
        bool connect(
            const char* ssid,
            const char* password
        );


        /**
         * @brief Intenta conectarse a una lista de redes Wi-Fi conocidas.
         *
         * Las redes se prueban en el mismo orden en que fueron entregadas.
         * El proceso termina cuando una conexión se establece correctamente.
         *
         * @param networks Arreglo de redes Wi-Fi conocidas.
         * @param count Cantidad de redes disponibles en el arreglo.
         *
         * @retval true Si fue posible conectarse a una de las redes.
         * @retval false Si ninguna red pudo ser utilizada.
         */
        bool connectToKnownNetworks(
            const WiFiNetwork* networks,
            uint8_t count
        );


        /**
         * @brief Sincroniza el reloj del dispositivo mediante NTP.
         *
         * Obtiene la fecha y hora UTC desde servidores NTP y actualiza
         * el reloj interno del ESP32-S3.
         *
         * Debe ejecutarse después de establecer una conexión Wi-Fi.
         *
         * @retval true Si la hora fue sincronizada correctamente.
         * @retval false Si no existe conexión Wi-Fi o no fue posible obtener una hora válida.
         */
        bool syncTime();


        /**
         * @brief Comprueba si el dispositivo se encuentra conectado a una red Wi-Fi.
         *
         * @retval true Si existe una conexión Wi-Fi STA activa.
         * @retval false Si el dispositivo está desconectado.
         */
        bool isConnected() const;


        /**
         * @brief Comprueba si un nombre de dominio puede resolverse mediante DNS.
         *
         * @param host Nombre del servidor que se desea resolver.
         *
         * @retval true Si el dominio fue resuelto correctamente.
         * @retval false Si no fue posible resolver el dominio.
         */
        bool testDns(
            const char* host
        );


        /**
         * @brief Desconecta únicamente la interfaz STA.
         *
         * El punto de acceso utilizado por el panel de configuración
         * permanece activo.
         */
        void disconnect();
};