#include <Arduino.h>
// =================================================================================================
#include "Config.h"
#include "DetectionSettings.h"
#include "WiFiNetwork.h"
#include "AlertData.h"
// =================================================================================================
#include "SettingsManager.h"
#include "SensorManager.h"
#include "NetworkManager.h"
#include "HotspotManager.h"
#include "WebConfigServer.h"
#include "SmokeGuard.h"
#include "ApiClient.h"
// =================================================================================================


namespace
{
    // =============================================================================================
    // DISPOSITIVO
    // =============================================================================================

    /**
     * @brief Identificador utilizado por la API para este dispositivo.
     *
     * Más adelante puede almacenarse en configuración o generarse
     * a partir del identificador único del ESP32-S3.
     */
    constexpr char DEVICE_ID[] = "smokeguard-001";


    // =============================================================================================
    // RED
    // =============================================================================================

    /**
     * @brief Intervalo entre nuevos intentos de preparar la conexión con la API.
     *
     * Solamente se utiliza cuando existe Wi-Fi pero NTP o DNS fallaron.
     */
    constexpr uint32_t NETWORK_PREPARATION_INTERVAL_MS = 30000;


    // =============================================================================================
    // COMPONENTES
    // =============================================================================================

    SettingsManager settingsManager;

    SensorManager sensorManager;

    NetworkManager networkManager;

    HotspotManager hotspotManager;

    WebConfigServer webConfigServer;

    SmokeGuard smokeGuard;

    ApiClient apiClient;


    // =============================================================================================
    // ESTADO DE COMPONENTES
    // =============================================================================================

    bool settingsAvailable = false;

    bool sensorsAvailable = false;

    bool hotspotAvailable = false;

    bool webServerAvailable = false;

    bool networkReady = false;


    // =============================================================================================
    // ESTADO DE RED
    // =============================================================================================

    /**
     * @brief Estado anterior de la conexión STA.
     *
     * Permite detectar conexiones y desconexiones sin consultar
     * continuamente NTP y DNS.
     */
    bool previousNetworkConnected = false;


    /**
     * @brief Momento del último intento de preparar la conexión con la API.
     */
    uint32_t lastNetworkPreparationMillis = 0;


    // =============================================================================================
    // ESTADO DE DETECCIÓN
    // =============================================================================================

    /**
     * @brief Estado anterior determinado por SmokeGuard.
     *
     * Las alertas se generan únicamente cuando existe una transición
     * relevante entre estados.
     */
    SmokeState previousSmokeState =
        SmokeState::InsufficientData;


    // =============================================================================================
    // ALERTA PENDIENTE
    // =============================================================================================

    /**
     * @brief Indica si existe una alerta esperando conexión con la API.
     */
    bool pendingAlertAvailable = false;


    /**
     * @brief Alerta pendiente de envío.
     */
    AlertData pendingAlert;


    /**
     * @brief Estado de SmokeGuard asociado a la alerta pendiente.
     *
     * Permite reemplazar una alerta SmokeProbable pendiente por una
     * SmokeHighConfidence si el evento aumenta de severidad antes de
     * recuperar la conexión.
     */
    SmokeState pendingAlertState =
        SmokeState::InsufficientData;


    // =============================================================================================
    // DIAGNÓSTICO
    // =============================================================================================

    /**
     * @brief Momento de la última impresión del estado del sistema.
     */
    uint32_t lastPrintMillis = 0;


    // =============================================================================================
    // CONVERTIR ESTADO A TEXTO
    // =============================================================================================

    const char* smokeStateToString(SmokeState state)
    {
        switch (state)
        {
            case SmokeState::InsufficientData:
                return "DATOS INSUFICIENTES";

            case SmokeState::WarmingUp:
                return "APRENDIENDO";

            case SmokeState::Normal:
                return "NORMAL";

            case SmokeState::Suspicious:
                return "SOSPECHOSO";

            case SmokeState::SmokeProbable:
                return "HUMO PROBABLE";

            case SmokeState::SmokeHighConfidence:
                return "HUMO ALTA CONFIANZA";

            default:
                return "DESCONOCIDO";
        }
    }


    // =============================================================================================
    // OBTENER SEVERIDAD DE ESTADO
    // =============================================================================================

    uint8_t smokeStateSeverity(SmokeState state)
    {
        switch (state)
        {
            case SmokeState::Suspicious:
                return 1;

            case SmokeState::SmokeProbable:
                return 2;

            case SmokeState::SmokeHighConfidence:
                return 3;

            default:
                return 0;
        }
    }


    // =============================================================================================
    // COMPROBAR ESTADO DE ALERTA
    // =============================================================================================

    bool isAlertState(SmokeState state)
    {
        return (
            state == SmokeState::SmokeProbable ||
            state == SmokeState::SmokeHighConfidence
        );
    }


    // =============================================================================================
    // PREPARAR CONEXIÓN PARA API
    // =============================================================================================

    bool prepareNetworkForApi()
    {
        if (!networkManager.isConnected())
        {
            networkReady = false;

            return false;
        }


        Serial0.println();
        Serial0.println("========================================");
        Serial0.println("          PREPARANDO RED");
        Serial0.println("========================================");


        // -----------------------------------------------------------------------------------------
        // DETENER TEMPORALMENTE DNS DEL PORTAL CAUTIVO
        // -----------------------------------------------------------------------------------------

        const bool dnsWasRunning =
            hotspotAvailable &&
            hotspotManager.isDnsRunning();


        if (dnsWasRunning)
        {
            hotspotManager.stopDns();
        }


        // -----------------------------------------------------------------------------------------
        // SINCRONIZAR HORA
        // -----------------------------------------------------------------------------------------

        const bool timeReady =
            networkManager.syncTime();


        // -----------------------------------------------------------------------------------------
        // COMPROBAR DNS DE LA API
        // -----------------------------------------------------------------------------------------

        bool apiDnsReady = false;


        if (timeReady)
        {
            apiDnsReady =
                networkManager.testDns(
                    Config::API_HOST
                );
        }


        // -----------------------------------------------------------------------------------------
        // RESTAURAR DNS DEL PORTAL CAUTIVO
        // -----------------------------------------------------------------------------------------

        if (dnsWasRunning)
        {
            hotspotManager.startDns();
        }


        // -----------------------------------------------------------------------------------------
        // RESULTADO
        // -----------------------------------------------------------------------------------------

        networkReady =
            timeReady &&
            apiDnsReady;


        if (networkReady)
        {
            Serial0.println(
                "[OK] Red preparada para comunicación HTTPS."
            );
        }
        else
        {
            Serial0.println(
                "[ADVERTENCIA] La API no está disponible actualmente."
            );
        }


        return networkReady;
    }


    // =============================================================================================
    // ACTUALIZAR ESTADO DE RED
    // =============================================================================================

    void updateNetworkState()
    {
        const bool connected =
            networkManager.isConnected();


        // -----------------------------------------------------------------------------------------
        // DETECTAR DESCONEXIÓN
        // -----------------------------------------------------------------------------------------

        if (!connected)
        {
            if (previousNetworkConnected)
            {
                Serial0.println(
                    "[WiFi] Conexión STA perdida."
                );
            }


            previousNetworkConnected = false;

            networkReady = false;

            return;
        }


        // -----------------------------------------------------------------------------------------
        // DETECTAR NUEVA CONEXIÓN
        // -----------------------------------------------------------------------------------------

        if (!previousNetworkConnected)
        {
            previousNetworkConnected = true;

            networkReady = false;

            lastNetworkPreparationMillis = 0;


            Serial0.println(
                "[WiFi] Nueva conexión STA detectada."
            );
        }


        // La conexión ya fue preparada.
        if (networkReady)
        {
            return;
        }


        // -----------------------------------------------------------------------------------------
        // CONTROLAR INTERVALO DE REINTENTO
        // -----------------------------------------------------------------------------------------

        const uint32_t currentMillis =
            millis();


        if (
            lastNetworkPreparationMillis != 0 &&
            currentMillis -
            lastNetworkPreparationMillis <
            NETWORK_PREPARATION_INTERVAL_MS
        )
        {
            return;
        }


        lastNetworkPreparationMillis =
            currentMillis;


        prepareNetworkForApi();
    }


    // =============================================================================================
    // CREAR ALERTA
    // =============================================================================================

    AlertData createSmokeAlert(
        const SmokeAnalysis& analysis
    )
    {
        AlertData alert;


        // -----------------------------------------------------------------------------------------
        // DISPOSITIVO
        // -----------------------------------------------------------------------------------------

        alert.deviceId =
            DEVICE_ID;


        // -----------------------------------------------------------------------------------------
        // ORIGEN
        // -----------------------------------------------------------------------------------------

        /*
         * La detección no pertenece exclusivamente a SPS30, SGP41 o SHT45.
         *
         * Es resultado de la fusión realizada por SmokeGuard.
         */
        alert.sensor =
            "SMOKEGUARD";


        // -----------------------------------------------------------------------------------------
        // TIPO Y SEVERIDAD
        // -----------------------------------------------------------------------------------------

        if (
            analysis.state ==
            SmokeState::SmokeHighConfidence
        )
        {
            alert.alertType =
                "SMOKE_HIGH_CONFIDENCE";

            alert.level =
                "critical";
        }
        else
        {
            alert.alertType =
                "SMOKE_PROBABLE";

            alert.level =
                "warning";
        }


        // -----------------------------------------------------------------------------------------
        // VALOR
        // -----------------------------------------------------------------------------------------

        /*
         * El valor enviado corresponde a la puntuación calculada
         * por el algoritmo de fusión de sensores.
         */
        alert.value =
            static_cast<float>(
                analysis.score
            );


        alert.unit =
            "score";


        return alert;
    }


    // =============================================================================================
    // GUARDAR ALERTA PENDIENTE
    // =============================================================================================

    void queueSmokeAlert(
        const SmokeAnalysis& analysis
    )
    {
        if (!isAlertState(analysis.state))
        {
            return;
        }


        /*
         * Si ya existe una alerta pendiente de mayor severidad,
         * no debe ser reemplazada por una alerta inferior.
         */
        if (
            pendingAlertAvailable &&
            smokeStateSeverity(analysis.state) <
            smokeStateSeverity(pendingAlertState)
        )
        {
            return;
        }


        pendingAlert =
            createSmokeAlert(
                analysis
            );


        pendingAlertState =
            analysis.state;


        pendingAlertAvailable =
            true;


        Serial0.printf(
            "[Alerta] Alerta preparada: %s\n",
            smokeStateToString(
                analysis.state
            )
        );
    }


    // =============================================================================================
    // PROCESAR CAMBIO DE ESTADO
    // =============================================================================================

    void processSmokeTransition(
        const SmokeAnalysis& analysis
    )
    {
        const SmokeState currentState =
            analysis.state;


        // -----------------------------------------------------------------------------------------
        // NO EXISTE CAMBIO
        // -----------------------------------------------------------------------------------------

        if (currentState == previousSmokeState)
        {
            return;
        }


        // -----------------------------------------------------------------------------------------
        // MOSTRAR TRANSICIÓN
        // -----------------------------------------------------------------------------------------

        Serial0.println();
        Serial0.println("========================================");
        Serial0.println("       CAMBIO DE ESTADO SMOKEGUARD");
        Serial0.println("========================================");


        Serial0.printf(
            "[SmokeGuard] %s -> %s\n",
            smokeStateToString(
                previousSmokeState
            ),
            smokeStateToString(
                currentState
            )
        );


        Serial0.printf(
            "[SmokeGuard] Score: %u/100\n",
            analysis.score
        );


        // -----------------------------------------------------------------------------------------
        // COMPROBAR AUMENTO DE SEVERIDAD
        // -----------------------------------------------------------------------------------------

        const uint8_t previousSeverity =
            smokeStateSeverity(
                previousSmokeState
            );


        const uint8_t currentSeverity =
            smokeStateSeverity(
                currentState
            );


        /*
         * Solamente generamos una alerta cuando:
         *
         * 1. El nuevo estado requiere alerta.
         * 2. La severidad aumentó.
         *
         * Ejemplos:
         *
         * Suspicious -> SmokeProbable          = alerta warning.
         * SmokeProbable -> SmokeHighConfidence = alerta critical.
         * SmokeHighConfidence -> SmokeProbable = no genera alerta.
         */
        if (
            isAlertState(currentState) &&
            currentSeverity > previousSeverity
        )
        {
            queueSmokeAlert(
                analysis
            );
        }


        // -----------------------------------------------------------------------------------------
        // GUARDAR ESTADO
        // -----------------------------------------------------------------------------------------

        previousSmokeState =
            currentState;
    }


    // =============================================================================================
    // ENVIAR ALERTA PENDIENTE
    // =============================================================================================

    void sendPendingAlert()
    {
        if (!pendingAlertAvailable)
        {
            return;
        }


        // -----------------------------------------------------------------------------------------
        // ESPERAR RED
        // -----------------------------------------------------------------------------------------

        if (
            !networkReady ||
            !networkManager.isConnected()
        )
        {
            return;
        }


        Serial0.println();
        Serial0.println("========================================");
        Serial0.println("           ALERTA PENDIENTE");
        Serial0.println("========================================");


        /*
         * Guardamos localmente el hecho de que el POST será intentado.
         *
         * Una vez iniciado sendAlert(), la alerta se elimina de la cola
         * aunque el resultado sea false.
         *
         * Esto evita un POST automático duplicado cuando la API recibió
         * correctamente el mensaje pero la respuesta HTTPS se perdió.
         */
        pendingAlertAvailable =
            false;


        const bool sent =
            apiClient.sendAlert(
                pendingAlert
            );


        if (sent)
        {
            Serial0.println(
                "[Alerta] Alerta registrada correctamente."
            );
        }
        else
        {
            Serial0.println(
                "[Alerta] No fue posible confirmar el envío."
            );

            Serial0.println(
                "[Alerta] No se realizará un reintento automático."
            );
        }
    }


    // =============================================================================================
    // MOSTRAR ESTADO
    // =============================================================================================

    void printStatus(
        const SensorData& data,
        const SmokeAnalysis& analysis
    )
    {
        const uint32_t currentMillis =
            millis();


        if (
            currentMillis -
            lastPrintMillis <
            Config::PRINT_INTERVAL_MS
        )
        {
            return;
        }


        lastPrintMillis =
            currentMillis;


        Serial0.println();
        Serial0.println("========================================");
        Serial0.println("          ESTADO SMOKEGUARD");
        Serial0.println("========================================");


        // -----------------------------------------------------------------------------------------
        // SHT45
        // -----------------------------------------------------------------------------------------

        if (data.sht45DataValid)
        {
            Serial0.printf(
                "Temperatura : %.2f C\n",
                data.temperature
            );


            Serial0.printf(
                "Humedad     : %.2f %%\n",
                data.humidity
            );
        }
        else
        {
            Serial0.println(
                "SHT45       : SIN DATOS"
            );
        }


        // -----------------------------------------------------------------------------------------
        // SGP41
        // -----------------------------------------------------------------------------------------

        if (data.sgp41VocDataValid)
        {
            Serial0.printf(
                "VOC Index   : %ld\n",
                static_cast<long>(
                    data.vocIndex
                )
            );
        }
        else
        {
            Serial0.println(
                "VOC Index   : SIN DATOS"
            );
        }


        if (data.sgp41NoxDataValid)
        {
            Serial0.printf(
                "NOx Index   : %ld\n",
                static_cast<long>(
                    data.noxIndex
                )
            );
        }
        else
        {
            Serial0.println(
                "NOx Index   : NO DISPONIBLE"
            );
        }


        // -----------------------------------------------------------------------------------------
        // SPS30
        // -----------------------------------------------------------------------------------------

        if (data.sps30DataValid)
        {
            Serial0.printf(
                "PM1.0       : %.2f ug/m3\n",
                data.pm1_0
            );


            Serial0.printf(
                "PM2.5       : %.2f ug/m3\n",
                data.pm2_5
            );


            Serial0.printf(
                "PM4.0       : %.2f ug/m3\n",
                data.pm4_0
            );


            Serial0.printf(
                "PM10        : %.2f ug/m3\n",
                data.pm10
            );
        }
        else
        {
            Serial0.println(
                "SPS30       : SIN DATOS"
            );
        }


        // -----------------------------------------------------------------------------------------
        // SMOKEGUARD
        // -----------------------------------------------------------------------------------------

        Serial0.println(
            "----------------------------------------"
        );


        Serial0.printf(
            "Estado      : %s\n",
            smokeStateToString(
                analysis.state
            )
        );


        Serial0.printf(
            "Score       : %u/100\n",
            analysis.score
        );


        Serial0.printf(
            "Delta PM2.5 : %.2f\n",
            analysis.pm25Delta
        );


        Serial0.printf(
            "Ratio fino  : %.2f\n",
            analysis.fineParticleRatio
        );


        Serial0.printf(
            "Delta VOC   : %.2f\n",
            analysis.vocDelta
        );


        Serial0.printf(
            "Delta NOx   : %.2f\n",
            analysis.noxDelta
        );


        Serial0.printf(
            "Delta RH    : %.2f\n",
            analysis.humidityDelta
        );


        // -----------------------------------------------------------------------------------------
        // EVIDENCIAS
        // -----------------------------------------------------------------------------------------

        Serial0.println(
            "----------------------------------------"
        );


        Serial0.printf(
            "PM          : %s\n",
            analysis.particleEvidence
                ? "SI"
                : "NO"
        );


        Serial0.printf(
            "Part. finas : %s\n",
            analysis.fineParticleEvidence
                ? "SI"
                : "NO"
        );


        Serial0.printf(
            "VOC         : %s\n",
            analysis.vocEvidence
                ? "SI"
                : "NO"
        );


        Serial0.printf(
            "NOx         : %s\n",
            analysis.noxEvidence
                ? "SI"
                : "NO"
        );


        Serial0.printf(
            "Persistencia: %s\n",
            analysis.persistenceEvidence
                ? "SI"
                : "NO"
        );


        // -----------------------------------------------------------------------------------------
        // RED
        // -----------------------------------------------------------------------------------------

        Serial0.println(
            "----------------------------------------"
        );


        Serial0.printf(
            "Hotspot     : %s\n",
            hotspotAvailable
                ? "ACTIVO"
                : "NO DISPONIBLE"
        );


        Serial0.printf(
            "Wi-Fi STA   : %s\n",
            networkManager.isConnected()
                ? "CONECTADO"
                : "DESCONECTADO"
        );


        Serial0.printf(
            "API         : %s\n",
            networkReady
                ? "DISPONIBLE"
                : "NO DISPONIBLE"
        );


        Serial0.printf(
            "Alerta cola : %s\n",
            pendingAlertAvailable
                ? "SI"
                : "NO"
        );
    }


    // =============================================================================================
    // CONECTAR A REDES ALMACENADAS
    // =============================================================================================

    void connectToStoredNetworks()
    {
        if (!settingsAvailable)
        {
            return;
        }


        // -----------------------------------------------------------------------------------------
        // CARGAR REDES
        // -----------------------------------------------------------------------------------------

        WiFiNetwork networks[
            Config::MAX_WIFI_NETWORKS
        ];


        const uint8_t networkCount =
            settingsManager.loadWiFiNetworks(
                networks,
                Config::MAX_WIFI_NETWORKS
            );


        if (networkCount == 0)
        {
            Serial0.println();
            Serial0.println(
                "[WiFi] No existen redes almacenadas."
            );

            Serial0.println(
                "[WiFi] Configure una red desde el panel local."
            );

            return;
        }


        // -----------------------------------------------------------------------------------------
        // DETENER TEMPORALMENTE DNS DEL PORTAL
        // -----------------------------------------------------------------------------------------

        const bool dnsWasRunning =
            hotspotAvailable &&
            hotspotManager.isDnsRunning();


        if (dnsWasRunning)
        {
            hotspotManager.stopDns();
        }


        // -----------------------------------------------------------------------------------------
        // INTENTAR CONEXIÓN
        // -----------------------------------------------------------------------------------------

        const bool connected =
            networkManager.connectToKnownNetworks(
                networks,
                networkCount
            );


        // -----------------------------------------------------------------------------------------
        // PREPARAR RED
        // -----------------------------------------------------------------------------------------

        if (connected)
        {
            previousNetworkConnected =
                true;


            const bool timeReady =
                networkManager.syncTime();


            bool apiDnsReady =
                false;


            if (timeReady)
            {
                apiDnsReady =
                    networkManager.testDns(
                        Config::API_HOST
                    );
            }


            networkReady =
                timeReady &&
                apiDnsReady;
        }
        else
        {
            previousNetworkConnected =
                false;

            networkReady =
                false;


            Serial0.println(
                "[WiFi] No fue posible conectar a ninguna red almacenada."
            );
        }


        // -----------------------------------------------------------------------------------------
        // RESTAURAR DNS DEL PORTAL
        // -----------------------------------------------------------------------------------------

        if (dnsWasRunning)
        {
            hotspotManager.startDns();
        }


        lastNetworkPreparationMillis =
            millis();
    }
}


// =================================================================================================
// SETUP
// =================================================================================================

void setup()
{
    // =============================================================================================
    // SERIAL
    // =============================================================================================

    Serial0.begin(115200);

    delay(1500);


    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("              SMOKEGUARD");
    Serial0.println("========================================");


    // =============================================================================================
    // CONFIGURACIÓN
    // =============================================================================================

    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("           CONFIGURACION");
    Serial0.println("========================================");


    settingsAvailable =
        settingsManager.begin();


    DetectionSettings detectionSettings;


    if (settingsAvailable)
    {
        detectionSettings =
            settingsManager.loadDetectionSettings();
    }
    else
    {
        Serial0.println(
            "[ADVERTENCIA] No fue posible acceder a NVS."
        );

        Serial0.println(
            "[ADVERTENCIA] Se utilizará la configuración de detección predeterminada."
        );
    }


    // =============================================================================================
    // SMOKEGUARD
    // =============================================================================================

    smokeGuard.begin(
        detectionSettings
    );


    // =============================================================================================
    // HOTSPOT
    // =============================================================================================

    hotspotAvailable =
        hotspotManager.begin();


    if (!hotspotAvailable)
    {
        Serial0.println(
            "[ADVERTENCIA] El hotspot local no está disponible."
        );
    }


    // =============================================================================================
    // SERVIDOR WEB
    // =============================================================================================

    webServerAvailable =
        webConfigServer.begin(
            hotspotManager,
            networkManager,
            settingsManager,
            smokeGuard
        );


    if (!webServerAvailable)
    {
        Serial0.println(
            "[ADVERTENCIA] El panel web no está disponible."
        );
    }


    // =============================================================================================
    // SENSORES
    // =============================================================================================

    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("              SENSORES");
    Serial0.println("========================================");


    sensorsAvailable =
        sensorManager.begin();


    if (!sensorsAvailable)
    {
        /*
         * No detenemos SmokeGuard.
         *
         * SensorManager puede seguir funcionando con los sensores
         * que sí hayan sido inicializados y el panel local debe
         * permanecer disponible para diagnóstico.
         */
        Serial0.println(
            "[ADVERTENCIA] Uno o más sensores no están disponibles."
        );
    }


    // =============================================================================================
    // RED STA
    // =============================================================================================

    connectToStoredNetworks();


    // =============================================================================================
    // SISTEMA INICIADO
    // =============================================================================================

    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("          SISTEMA INICIADO");
    Serial0.println("========================================");


    if (hotspotAvailable)
    {
        Serial0.printf(
            "[AP] SSID: %s\n",
            Config::AP_SSID
        );


        Serial0.printf(
            "[Panel] http://%s/\n",
            hotspotManager
                .getIp()
                .toString()
                .c_str()
        );
    }


    Serial0.printf(
        "[Sensores] %s\n",
        sensorsAvailable
            ? "OK"
            : "PARCIAL"
    );


    Serial0.printf(
        "[WiFi STA] %s\n",
        networkManager.isConnected()
            ? "CONECTADO"
            : "DESCONECTADO"
    );


    Serial0.printf(
        "[API] %s\n",
        networkReady
            ? "DISPONIBLE"
            : "NO DISPONIBLE"
    );
}


// =================================================================================================
// LOOP
// =================================================================================================

void loop()
{
    // =============================================================================================
    // HOTSPOT Y PORTAL CAUTIVO
    // =============================================================================================

    if (hotspotAvailable)
    {
        hotspotManager.update();
    }


    // =============================================================================================
    // SERVIDOR WEB
    // =============================================================================================

    if (webServerAvailable)
    {
        webConfigServer.update();
    }


    // =============================================================================================
    // ESTADO DE RED
    // =============================================================================================

    updateNetworkState();


    // =============================================================================================
    // SENSORES
    // =============================================================================================

    sensorManager.update();


    const SensorData& data =
        sensorManager.getData();


    // =============================================================================================
    // DETECCIÓN
    // =============================================================================================

    const SmokeAnalysis analysis =
        smokeGuard.analyze(
            data
        );


    // =============================================================================================
    // CAMBIOS DE ESTADO Y ALERTAS
    // =============================================================================================

    processSmokeTransition(
        analysis
    );


    // =============================================================================================
    // ENVIAR ALERTA PENDIENTE
    // =============================================================================================

    sendPendingAlert();


    // =============================================================================================
    // DIAGNÓSTICO
    // =============================================================================================

    printStatus(
        data,
        analysis
    );
}