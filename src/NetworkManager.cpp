#include <time.h>
// =================================================================================================
#include "NetworkManager.h"
#include "Config.h"
// =================================================================================================


// -------------------------------------------------------------------------------------------------
// CONECTAR A RED WI-FI
// -------------------------------------------------------------------------------------------------

bool NetworkManager::connect(const char* ssid, const char* password)
{
    // =============================================================================================
    // VALIDAR PARÁMETROS
    // =============================================================================================

    if (ssid == nullptr || ssid[0] == '\0')
    {
        Serial0.println("[WiFi] ERROR: SSID no válido.");
        return false;
    }

    /*
     * Una contraseña nullptr se interpreta como una contraseña vacía.
     *
     * Esto permite también conectarse a redes abiertas.
     */
    if (password == nullptr)
    {
        password = "";
    }

    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("          CONEXION WI-FI");
    Serial0.println("========================================");


    // =============================================================================================
    // CONFIGURAR WI-FI
    // =============================================================================================

    /*
     * Mantener simultáneamente:
     *
     * AP  -> panel de configuración SmokeGuard.
     * STA -> conexión con router o punto de acceso externo.
     *
     * WiFi.mode(WIFI_STA) no debe utilizarse aquí porque podría
     * desactivar el AP de configuración.
     */
    WiFi.mode(WIFI_AP_STA);

    // Desactivar ahorro de energía para mejorar la estabilidad.
    WiFi.setSleep(false);

    // =============================================================================================
    // COMPROBAR CONEXIÓN EXISTENTE
    // =============================================================================================

    if (WiFi.status() == WL_CONNECTED)
    {
        /*
         * Si ya estamos conectados exactamente a la red solicitada,
         * no necesitamos iniciar una nueva conexión.
         */
        if (WiFi.SSID() == ssid)
        {
            Serial0.printf("[WiFi] Ya conectado a: %s\n", ssid);
            return true;
        }

        /*
         * Desconectar solamente la interfaz STA.
         *
         * false = no apagar la interfaz Wi-Fi.
         * false = no borrar la configuración Wi-Fi persistida por el SDK.
         *
         * El AP permanece activo.
         */
        WiFi.disconnect(false, false);

        delay(100);
    }


    // =============================================================================================
    // INICIAR CONEXIÓN
    // =============================================================================================

    Serial0.printf("[WiFi] Conectando a: %s\n", ssid);

    WiFi.begin(ssid, password);

    const uint32_t startTime = millis();

    // =============================================================================================
    // ESPERAR CONEXIÓN
    // =============================================================================================

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < Config::STA_CONNECT_TIMEOUT_MS)
    {
        Serial0.print(".");
        delay(500);
    }

    Serial0.println();

    // =============================================================================================
    // COMPROBAR RESULTADO
    // =============================================================================================

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial0.printf(
            "[WiFi] ERROR: no fue posible conectar a '%s'. Estado: %d\n",
            ssid,
            WiFi.status()
        );

        /*
         * Detener cualquier intento pendiente de la interfaz STA.
         *
         * El AP continúa disponible.
         */
        WiFi.disconnect(false, false);

        return false;
    }

    // =============================================================================================
    // MOSTRAR INFORMACIÓN
    // =============================================================================================

    Serial0.println("[OK] Wi-Fi conectado.");

    Serial0.printf("[WiFi] SSID: %s\n", WiFi.SSID().c_str());

    Serial0.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());

    Serial0.printf("[WiFi] Gateway: %s\n", WiFi.gatewayIP().toString().c_str());

    Serial0.printf("[WiFi] DNS: %s\n", WiFi.dnsIP().toString().c_str());

    Serial0.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());

    return true;
}


// -------------------------------------------------------------------------------------------------
// CONECTAR A REDES WI-FI CONOCIDAS
// -------------------------------------------------------------------------------------------------

bool NetworkManager::connectToKnownNetworks(const WiFiNetwork* networks, uint8_t count)
{
    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("       REDES WI-FI CONOCIDAS");
    Serial0.println("========================================");


    // =============================================================================================
    // VALIDAR LISTA
    // =============================================================================================

    if (networks == nullptr || count == 0)
    {
        Serial0.println("[WiFi] No existen redes Wi-Fi configuradas.");
        return false;
    }

    Serial0.printf("[WiFi] Redes disponibles: %u\n", count);


    // =============================================================================================
    // PROBAR REDES
    // =============================================================================================

    for (uint8_t index = 0; index < count; index++)
    {
        const WiFiNetwork& network = networks[index];

        if (network.ssid.isEmpty())
        {
            continue;
        }

        Serial0.println();

        Serial0.printf(
            "[WiFi] Intento %u de %u: %s\n",
            index + 1,
            count,
            network.ssid.c_str()
        );

        /*
         * La contraseña ya fue descifrada previamente por SettingsManager.
         *
         * NetworkManager no conoce ni necesita conocer cómo se almacenan
         * las credenciales.
         */
        if (connect(network.ssid.c_str(), network.password.c_str()))
        {
            Serial0.printf("[WiFi] Red seleccionada: %s\n", network.ssid.c_str());
            return true;
        }
    }


    // =============================================================================================
    // NINGUNA RED DISPONIBLE
    // =============================================================================================

    Serial0.println();

    Serial0.println("[WiFi] No fue posible conectar a ninguna red conocida.");

    /*
     * Esto no representa un fallo crítico.
     *
     * SmokeGuard puede continuar ejecutándose y el AP de configuración
     * permanece disponible para que el usuario agregue o modifique redes.
     */
    return false;
}


// -------------------------------------------------------------------------------------------------
// SINCRONIZAR HORA MEDIANTE NTP
// -------------------------------------------------------------------------------------------------

bool NetworkManager::syncTime()
{
    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("        SINCRONIZACION DE HORA");
    Serial0.println("========================================");


    // =============================================================================================
    // COMPROBAR CONEXIÓN WI-FI
    // =============================================================================================

    if (!isConnected())
    {
        Serial0.println("[NTP] ERROR: no existe una conexión Wi-Fi activa.");
        return false;
    }


    Serial0.println("[NTP] Sincronizando reloj...");


    /*
     * Configurar servidores NTP.
     *
     * GMT offset      = 0
     * Daylight offset = 0
     *
     * El reloj del sistema permanece en UTC, que es lo apropiado
     * para la validación de certificados TLS.
     */

    configTime(0, 0, Config::NTP_SERVER_1, Config::NTP_SERVER_2, Config::NTP_SERVER_3);

    // =============================================================================================
    // ESPERAR SINCRONIZACIÓN
    // =============================================================================================

    struct tm timeInfo = {};

    if (!getLocalTime(&timeInfo, Config::NTP_SYNC_TIMEOUT_MS))
    {
        Serial0.println("[NTP] ERROR: no fue posible sincronizar la hora.");

        return false;
    }

    // =============================================================================================
    // COMPROBAR FECHA
    // =============================================================================================

    const uint16_t year = timeInfo.tm_year + 1900;

    if (year < Config::NTP_MIN_VALID_YEAR)
    {
        Serial0.printf("[NTP] ERROR: fecha no válida. Año recibido: %u\n", year);

        return false;
    }

    // =============================================================================================
    // MOSTRAR HORA UTC
    // =============================================================================================

    Serial0.printf(
        "[NTP] UTC: %04u-%02u-%02u %02u:%02u:%02u\n",
        year,
        timeInfo.tm_mon + 1,
        timeInfo.tm_mday,
        timeInfo.tm_hour,
        timeInfo.tm_min,
        timeInfo.tm_sec
    );

    Serial0.println("[OK] Reloj sincronizado.");
    return true;
}

// -------------------------------------------------------------------------------------------------
// COMPROBAR CONEXIÓN WI-FI
// -------------------------------------------------------------------------------------------------

bool NetworkManager::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

// -------------------------------------------------------------------------------------------------
// COMPROBAR RESOLUCIÓN DNS
// -------------------------------------------------------------------------------------------------

bool NetworkManager::testDns(const char* host)
{
    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("             PRUEBA DNS");
    Serial0.println("========================================");


    // =============================================================================================
    // VALIDAR HOST
    // =============================================================================================

    if (host == nullptr || host[0] == '\0')
    {
        Serial0.println("[DNS] ERROR: dominio no válido.");
        return false;
    }

    // =============================================================================================
    // COMPROBAR CONEXIÓN
    // =============================================================================================

    if (!isConnected())
    {
        Serial0.println("[DNS] ERROR: no existe una conexión Wi-Fi activa.");
        return false;
    }

    // =============================================================================================
    // RESOLVER DOMINIO
    // =============================================================================================

    IPAddress serverIp;

    Serial0.printf("[DNS] Resolviendo %s...\n", host);

    const int result = WiFi.hostByName(host, serverIp);

    if (result != 1)
    {
        Serial0.println("[DNS] ERROR: no fue posible resolver el dominio.");
        return false;
    }

    Serial0.printf("[OK] Dirección IP: %s\n", serverIp.toString().c_str());
    return true;
}


// -------------------------------------------------------------------------------------------------
// DESCONECTAR DE RED WI-FI
// -------------------------------------------------------------------------------------------------

void NetworkManager::disconnect()
{
    if (!isConnected())
    {
        return;
    }

    /*
     * Desconectar únicamente la interfaz STA.
     *
     * El primer false evita apagar la interfaz Wi-Fi.
     * El segundo false evita borrar información persistida por el SDK.
     *
     * El AP utilizado por el panel continúa funcionando.
     */
    WiFi.disconnect(false, false);
    Serial0.println( "[WiFi] Interfaz STA desconectada.");
}