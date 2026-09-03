#pragma once
// =================================================================================================
#include <Arduino.h>
#include <DNSServer.h>
#include <WiFi.h>
// =================================================================================================
/**
 * @class HotspotManager
 * @file HotspotManager.h
 * @brief Administra el punto de acceso Wi-Fi y el portal cautivo de SmokeGuard.
 *
 * Mantiene disponible una red Wi-Fi local para acceder al panel de configuración
 * del dispositivo.
 *
 * SmokeGuard utiliza el modo WIFI_AP_STA, permitiendo mantener activo el punto
 * de acceso mientras el dispositivo se conecta simultáneamente a una red Wi-Fi
 * externa mediante la interfaz STA.
 *
 * También administra el servidor DNS utilizado por el portal cautivo.
 */
class HotspotManager
{
    public:

        /**
         * @brief Inicializa el punto de acceso de SmokeGuard.
         *
         * Configura la dirección IP del punto de acceso, habilita el modo
         * WIFI_AP_STA, inicia la red Wi-Fi de configuración y activa
         * el servidor DNS del portal cautivo.
         *
         * @retval true Si el punto de acceso fue iniciado correctamente.
         * @retval false Si ocurrió un error durante la inicialización.
         */
        bool begin();

        /**
         * @brief Actualiza el servidor DNS del portal cautivo.
         *
         * Debe ejecutarse periódicamente desde loop() para procesar
         * las solicitudes DNS recibidas.
         */
        void update();

        /**
         * @brief Inicia el servidor DNS del portal cautivo.
         *
         * Todas las consultas DNS se redirigen hacia la dirección IP
         * del punto de acceso de SmokeGuard.
         *
         * @retval true Si el servidor DNS fue iniciado correctamente.
         * @retval false Si ocurrió un error.
         */
        bool startDns();

        /**
         * @brief Detiene el servidor DNS del portal cautivo.
         *
         * Puede utilizarse temporalmente antes de realizar operaciones
         * sobre la interfaz Wi-Fi STA que puedan afectar al socket UDP
         * utilizado internamente por DNSServer.
         */
        void stopDns();

        /**
         * @brief Comprueba si el servidor DNS se encuentra activo.
         *
         * @retval true Si el servidor DNS está funcionando.
         * @retval false Si el servidor DNS está detenido.
         */
        bool isDnsRunning() const;

        /**
         * @brief Obtiene la dirección IP del punto de acceso.
         *
         * @return Dirección IP utilizada por el AP de SmokeGuard.
         */
        IPAddress getIp() const;


    private:

        /**
         * @brief Servidor DNS utilizado por el portal cautivo.
         */
        DNSServer dnsServer;

        /**
         * @brief Indica si el servidor DNS se encuentra actualmente activo.
         */
        bool dnsServerRunning = false;

        /**
         * @brief Dirección IP utilizada por el punto de acceso.
         */
        IPAddress apIp = IPAddress( 192, 168, 4, 1);

        /**
         * @brief Dirección de gateway utilizada por el punto de acceso.
         */
        IPAddress apGateway = IPAddress(192, 168, 4, 1);

        /**
         * @brief Máscara de subred utilizada por el punto de acceso.
         */
        IPAddress apSubnet = IPAddress( 255, 255, 255, 0);
};