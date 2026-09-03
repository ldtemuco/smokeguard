#include "WebConfigServer.h"
// =================================================================================================
#include "Config.h"
#include "DetectionSettings.h"
#include "HotspotManager.h"
#include "NetworkManager.h"
#include "SettingsManager.h"
#include "SmokeGuard.h"
#include "WiFiNetwork.h"
// =================================================================================================
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_system.h>
#include <cerrno>
#include <cstdlib>
// =================================================================================================


namespace
{
    // =============================================================================================
    // UTILIDADES DE CONVERSIÓN
    // =============================================================================================

    bool parseUnsignedInteger(const String& value, uint32_t& result)
    {
        if (value.isEmpty())
        {
            return false;
        }

        errno = 0;

        char* end = nullptr;

        const unsigned long parsed = strtoul(value.c_str(), &end, 10);

        if (errno == ERANGE || end == value.c_str() || *end != '\0')
        {
            return false;
        }

        result = static_cast<uint32_t>(parsed);

        return true;
    }


    bool parseFloatValue(const String& value, float& result)
    {
        if (value.isEmpty())
        {
            return false;
        }

        errno = 0;

        char* end = nullptr;

        const float parsed = strtof(value.c_str(), &end);

        if (errno == ERANGE ||end == value.c_str() || *end != '\0')
        {
            return false;
        }

        result = parsed;

        return true;
    }
}


// -------------------------------------------------------------------------------------------------
// CONSTRUCTOR
// -------------------------------------------------------------------------------------------------

WebConfigServer::WebConfigServer() : server(Config::HTTP_PORT)
{
}


// -------------------------------------------------------------------------------------------------
// INICIAR SERVIDOR WEB
// -------------------------------------------------------------------------------------------------

bool WebConfigServer::begin(
    HotspotManager& hotspotManager,
    NetworkManager& networkManager,
    SettingsManager& settingsManager,
    SmokeGuard& smokeGuard
)
{
    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("      SERVIDOR WEB DE CONFIGURACION");
    Serial0.println("========================================");


    // ---------------------------------------------------------------------------------------------
    // GUARDAR DEPENDENCIAS
    // ---------------------------------------------------------------------------------------------

    this->hotspotManager = &hotspotManager;

    this->networkManager = &networkManager;

    this->settingsManager = &settingsManager;

    this->smokeGuard = &smokeGuard;

    // ---------------------------------------------------------------------------------------------
    // INICIAR LITTLEFS
    // ---------------------------------------------------------------------------------------------

    /*
     * Conservamos la configuración utilizada en el prototipo funcional.
     *
     * formatOnFail  = true
     * basePath      = /littlefs
     * maxOpenFiles  = 10
     * partitionName = spiffs
     */
    if (!LittleFS.begin(true, "/littlefs", 10, "spiffs"))
    {
        Serial0.println("[LittleFS] ERROR: no fue posible montar el sistema de archivos.");
        return false;
    }

    Serial0.println("[LittleFS] Sistema de archivos montado.");

    // ---------------------------------------------------------------------------------------------
    // CONFIGURAR RUTAS
    // ---------------------------------------------------------------------------------------------

    configureRoutes();

    // ---------------------------------------------------------------------------------------------
    // INICIAR SERVIDOR
    // ---------------------------------------------------------------------------------------------

    server.begin();

    running = true;

    Serial0.printf("[HTTP] Servidor iniciado en puerto %u.\n", Config::HTTP_PORT);

    return true;
}


// -------------------------------------------------------------------------------------------------
// ACTUALIZAR SERVIDOR WEB
// -------------------------------------------------------------------------------------------------

void WebConfigServer::update()
{
    if (!running)
    {
        return;
    }

    server.handleClient();
}


// -------------------------------------------------------------------------------------------------
// CONFIGURAR RUTAS
// -------------------------------------------------------------------------------------------------

void WebConfigServer::configureRoutes()
{
    // =============================================================================================
    // ENCABEZADOS
    // =============================================================================================

    /*
     * WebServer solamente conserva los encabezados solicitados mediante
     * collectHeaders().
     *
     * Necesitamos Cookie para validar las sesiones.
     */
    const char* headerKeys[] =
    {
        "Cookie"
    };

    server.collectHeaders(headerKeys, 1);

    // =============================================================================================
    // PÁGINAS
    // =============================================================================================

    server.on(
        "/",
        HTTP_GET,
        [this]()
        {
            handleRoot();
        }
    );

    server.on(
        "/login",
        HTTP_GET,
        [this]()
        {
            handleLoginPage();
        }
    );

    server.on(
        "/dashboard",
        HTTP_GET,
        [this]()
        {
            handleProtectedPage(
                "/dashboard.html"
            );
        }
    );

    server.on(
        "/network",
        HTTP_GET,
        [this]()
        {
            handleProtectedPage(
                "/network.html"
            );
        }
    );

    server.on(
        "/configuration",
        HTTP_GET,
        [this]()
        {
            handleProtectedPage(
                "/configuration.html"
            );
        }
    );

    server.on(
        "/system",
        HTTP_GET,
        [this]()
        {
            handleProtectedPage(
                "/system.html"
            );
        }
    );

    server.on(
        "/logout",
        HTTP_GET,
        [this]()
        {
            handleLogout();
        }
    );


    // =============================================================================================
    // API - AUTENTICACIÓN
    // =============================================================================================

    server.on(
        "/api/login",
        HTTP_POST,
        [this]()
        {
            handleApiLogin();
        }
    );

    server.on(
        "/api/password",
        HTTP_POST,
        [this]()
        {
            handleApiPassword();
        }
    );

    // =============================================================================================
    // API - ESTADO
    // =============================================================================================

    server.on(
        "/api/status",
        HTTP_GET,
        [this]()
        {
            handleApiStatus();
        }
    );

    // =============================================================================================
    // API - WI-FI
    // =============================================================================================

    server.on(
        "/api/networks",
        HTTP_GET,
        [this]()
        {
            handleApiNetworks();
        }
    );

    server.on(
        "/api/wifi/saved",
        HTTP_GET,
        [this]()
        {
            handleApiSavedNetworks();
        }
    );

    server.on(
        "/api/wifi",
        HTTP_POST,
        [this]()
        {
            handleApiSaveWiFi();
        }
    );

    server.on(
        "/api/wifi/delete",
        HTTP_POST,
        [this]()
        {
            handleApiDeleteWiFi();
        }
    );

    // =============================================================================================
    // API - DETECCIÓN
    // =============================================================================================

    server.on(
        "/api/config/detection",
        HTTP_GET,
        [this]()
        {
            handleApiDetectionSettings();
        }
    );

    server.on(
        "/api/config/detection",
        HTTP_POST,
        [this]()
        {
            handleApiSaveDetectionSettings();
        }
    );

    server.on(
        "/api/config/detection/reset",
        HTTP_POST,
        [this]()
        {
            handleApiResetDetectionSettings();
        }
    );

    server.on(
        "/api/config/baseline/reset",
        HTTP_POST,
        [this]()
        {
            handleApiResetBaseline();
        }
    );

    // =============================================================================================
    // API - SISTEMA
    // =============================================================================================

    server.on(
        "/api/restart",
        HTTP_POST,
        [this]()
        {
            handleApiRestart();
        }
    );

    server.on(
        "/api/factory-reset",
        HTTP_POST,
        [this]()
        {
            handleApiFactoryReset();
        }
    );

    // =============================================================================================
    // ARCHIVOS ESTÁTICOS
    // =============================================================================================

    server.serveStatic(
        "/assets/",
        LittleFS,
        "/assets/"
    );

    // =============================================================================================
    // PORTAL CAUTIVO
    // =============================================================================================

    server.on(
        "/generate_204",
        HTTP_GET,
        [this]()
        {
            handleCaptivePortalProbe();
        }
    );

    server.on(
        "/hotspot-detect.html",
        HTTP_GET,
        [this]()
        {
            handleCaptivePortalProbe();
        }
    );

    server.on(
        "/connecttest.txt",
        HTTP_GET,
        [this]()
        {
            handleCaptivePortalProbe();
        }
    );

    server.on(
        "/ncsi.txt",
        HTTP_GET,
        [this]()
        {
            handleCaptivePortalProbe();
        }
    );

    // =============================================================================================
    // RUTAS NO ENCONTRADAS
    // =============================================================================================

    server.onNotFound([this](){handleCaptivePortalProbe();});
}


// -------------------------------------------------------------------------------------------------
// GENERAR TOKEN DE SESIÓN
// -------------------------------------------------------------------------------------------------

String WebConfigServer::generateSessionToken() const
{
    char buffer[65]{};

    /*
     * 32 bytes aleatorios representados en hexadecimal producen
     * un token de 64 caracteres.
     */

    for (size_t index = 0; index < 32; index++)
    {
        const uint8_t randomByte = static_cast<uint8_t>(esp_random() & 0xFFU);

        snprintf(&buffer[index * 2], 3, "%02x", randomByte);
    }

    return String(buffer);
}

// -------------------------------------------------------------------------------------------------
// COMPROBAR EXPIRACIÓN DE SESIÓN
// -------------------------------------------------------------------------------------------------

bool WebConfigServer::sessionExpired() const
{
    if (sessionToken.isEmpty())
    {
        return true;
    }

    return (millis() - sessionLastActivityMs > Config::SESSION_TIMEOUT_MS);
}


// -------------------------------------------------------------------------------------------------
// COMPROBAR AUTENTICACIÓN
// -------------------------------------------------------------------------------------------------

bool WebConfigServer::isAuthenticated()
{
    if (sessionExpired())
    {
        clearSession();

        return false;
    }

    const String cookie = server.header("Cookie");

    const String expected = "ESPSESSION=" + sessionToken;

    if (cookie.indexOf(expected) < 0)
    {
        return false;
    }

    sessionLastActivityMs = millis();

    return true;
}


// -------------------------------------------------------------------------------------------------
// EXIGIR AUTENTICACIÓN API
// -------------------------------------------------------------------------------------------------

bool WebConfigServer::requireApiAuth()
{
    if (isAuthenticated())
    {
        return true;
    }

    sendJson(401, R"({"ok":false,"error":"No autorizado"})");

    return false;
}

// -------------------------------------------------------------------------------------------------
// ELIMINAR SESIÓN
// -------------------------------------------------------------------------------------------------

void WebConfigServer::clearSession()
{
    sessionToken = "";

    sessionLastActivityMs = 0;
}


// -------------------------------------------------------------------------------------------------
// ENVIAR ENCABEZADOS SIN CACHÉ
// -------------------------------------------------------------------------------------------------

void WebConfigServer::sendNoCacheHeaders()
{
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");

    server.sendHeader("Pragma", "no-cache");
}


// -------------------------------------------------------------------------------------------------
// ENVIAR JSON
// -------------------------------------------------------------------------------------------------

void WebConfigServer::sendJson(int statusCode, const String& json)
{
    sendNoCacheHeaders();

    server.send(statusCode, "application/json; charset=utf-8", json);
}

// -------------------------------------------------------------------------------------------------
// REDIRIGIR
// -------------------------------------------------------------------------------------------------

void WebConfigServer::redirectTo(const String& location)
{
    server.sendHeader("Location", location, true);

    server.send(302, "text/plain", "");
}


// -------------------------------------------------------------------------------------------------
// OBTENER CONTENT-TYPE
// -------------------------------------------------------------------------------------------------

String WebConfigServer::contentTypeForPath(const String& path) const
{
    if (path.endsWith(".html"))
    {
        return "text/html; charset=utf-8";
    }

    if (path.endsWith(".css"))
    {
        return "text/css; charset=utf-8";
    }

    if (path.endsWith(".js"))
    {
        return "application/javascript; charset=utf-8";
    }

    if (path.endsWith(".json"))
    {
        return "application/json; charset=utf-8";
    }

    if (path.endsWith(".svg"))
    {
        return "image/svg+xml";
    }

    if (path.endsWith(".png"))
    {
        return "image/png";
    }

    if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
    {
        return "image/jpeg";
    }

    if (path.endsWith(".ico"))
    {
        return "image/x-icon";
    }

    return "application/octet-stream";
}


// -------------------------------------------------------------------------------------------------
// ENVIAR ARCHIVO LITTLEFS
// -------------------------------------------------------------------------------------------------

bool WebConfigServer::sendFile(const String& path)
{
    if (!LittleFS.exists(path))
    {
        server.send(404, "text/plain; charset=utf-8", "Archivo no encontrado");

        return false;
    }

    File file = LittleFS.open(path, "r");

    if (!file)
    {
        server.send(500, "text/plain; charset=utf-8", "No fue posible abrir el archivo");
        return false;
    }

    server.streamFile(file, contentTypeForPath(path));

    file.close();

    return true;
}

// -------------------------------------------------------------------------------------------------
// ESCAPAR JSON
// -------------------------------------------------------------------------------------------------

String WebConfigServer::jsonEscape(const String& value) const
{
    String escaped;

    escaped.reserve(value.length() + 8);

    for (size_t index = 0; index < value.length(); index++)
    {
        const char character = value[index];

        switch (character)
        {
            case '"':
                escaped += "\\\"";
                break;

            case '\\':
                escaped += "\\\\";
                break;

            case '\n':
                escaped += "\\n";
                break;

            case '\r':
                escaped += "\\r";
                break;

            case '\t':
                escaped += "\\t";
                break;

            default:

                if (static_cast<uint8_t>(character) >= 0x20)
                {
                    escaped += character;
                }

                break;
        }
    }

    return escaped;
}


// -------------------------------------------------------------------------------------------------
// PÁGINA RAÍZ
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleRoot()
{
    redirectTo(isAuthenticated() ? "/dashboard" : "/login");
}


// -------------------------------------------------------------------------------------------------
// PÁGINA LOGIN
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleLoginPage()
{
    if (isAuthenticated())
    {
        redirectTo("/dashboard");
        return;
    }

    sendNoCacheHeaders();

    sendFile("/login.html");
}


// -------------------------------------------------------------------------------------------------
// PÁGINA PROTEGIDA
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleProtectedPage(
    const char* filePath
)
{
    if (!isAuthenticated())
    {
        redirectTo("/login");

        return;
    }

    sendNoCacheHeaders();

    sendFile(filePath);
}


// -------------------------------------------------------------------------------------------------
// CERRAR SESIÓN
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleLogout()
{
    clearSession();

    server.sendHeader(
        "Set-Cookie", "ESPSESSION=deleted; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");

    redirectTo("/login");
}


// -------------------------------------------------------------------------------------------------
// API - LOGIN
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiLogin()
{
    const String username = server.arg("username");
    const String password = server.arg("password");

    if (!settingsManager->verifyAdminCredentials(username, password))
    {
        delay(250);

        sendJson(401, R"({"ok":false,"error":"Usuario o contraseña incorrectos"})");
        return;
    }

    sessionToken = generateSessionToken();

    sessionLastActivityMs = millis();

    server.sendHeader(
        "Set-Cookie",
        "ESPSESSION=" +
        sessionToken +
        "; Path=/; HttpOnly; SameSite=Strict"
    );

    sendJson(200, R"({"ok":true})");
}

// -------------------------------------------------------------------------------------------------
// API - CAMBIAR CONTRASEÑA
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiPassword()
{
    if (!requireApiAuth())
    {
        return;
    }

    const String currentPassword = server.arg("current_password");

    const String newPassword = server.arg("new_password");

    if (newPassword.length() < 8)
    {
        sendJson(
            400,
            R"({"ok":false,"error":"La nueva contraseña debe tener al menos 8 caracteres"})"
        );

        return;
    }

    if (!settingsManager->changeAdminPassword(currentPassword, newPassword))
    {
        sendJson(
            403,
            R"({"ok":false,"error":"La contraseña actual no es correcta"})"
        );

        return;
    }


    /*
     * Crear una nueva sesión después de cambiar la contraseña.
     */
    sessionToken = generateSessionToken();


    sessionLastActivityMs = millis();

    server.sendHeader(
        "Set-Cookie",
        "ESPSESSION=" +
        sessionToken +
        "; Path=/; HttpOnly; SameSite=Strict"
    );

    sendJson(200, R"({"ok":true})");
}


// -------------------------------------------------------------------------------------------------
// API - ESTADO
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiStatus()
{
    if (!requireApiAuth())
    {
        return;
    }

    const bool connected = networkManager->isConnected();

    String json;

    json.reserve(512);

    json += "{";

    json += "\"ok\":true,";

    json += "\"ap_ssid\":\"";
    json += jsonEscape(Config::AP_SSID);
    json += "\",";

    json += "\"ap_ip\":\"";
    json += hotspotManager->getIp().toString();
    json += "\",";

    json += "\"sta_connected\":";
    json += connected ? "true" : "false";
    json += ",";

    json += "\"sta_ssid\":\"";
    json += jsonEscape(connected ? WiFi.SSID() : String(""));
    json += "\",";

    json += "\"sta_ip\":\"";
    json += connected ? WiFi.localIP().toString() : String("-");
    json += "\",";

    json += "\"rssi\":";
    json += String(connected ? WiFi.RSSI() : 0);
    json += ",";

    json += "\"uptime_s\":";
    json += String(millis() / 1000UL);
    json += ",";

    json += "\"free_heap\":";
    json += String(ESP.getFreeHeap());
    json += ",";

    json += "\"flash_bytes\":";
    json += String(ESP.getFlashChipSize());
    json += ",";

    json += "\"psram_bytes\":";
    json += String(ESP.getPsramSize());

    json += "}";

    sendJson(200, json);
}


// -------------------------------------------------------------------------------------------------
// API - ESCANEAR REDES WI-FI
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiNetworks()
{
    if (!requireApiAuth())
    {
        return;
    }


    const int count =
        WiFi.scanNetworks();


    if (count < 0)
    {
        sendJson(
            500,
            R"({"ok":false,"error":"No fue posible escanear las redes Wi-Fi"})"
        );


        return;
    }


    String json =
        "{\"ok\":true,\"networks\":[";


    for (int index = 0; index < count; index++)
    {
        if (index > 0)
        {
            json += ",";
        }

        const bool secure =
            WiFi.encryptionType(index)
            !=
            WIFI_AUTH_OPEN;

        json += "{";


        json += "\"ssid\":\"";
        json += jsonEscape(WiFi.SSID(index));
        json += "\",";

        json += "\"rssi\":";
        json += String(WiFi.RSSI(index));
        json += ",";

        json += "\"secure\":";
        json += secure ? "true" : "false";

        json += "}";
    }

    json += "]}";

    WiFi.scanDelete();

    sendJson(200, json);
}


// -------------------------------------------------------------------------------------------------
// API - REDES WI-FI GUARDADAS
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiSavedNetworks()
{
    if (!requireApiAuth())
    {
        return;
    }

    WiFiNetwork networks[Config::MAX_WIFI_NETWORKS];

    const uint8_t count = settingsManager->loadWiFiNetworks(networks, Config::MAX_WIFI_NETWORKS);

    const bool connected = networkManager->isConnected();

    const String currentSsid = connected ? WiFi.SSID() : String("");

    String json = "{\"ok\":true,\"networks\":[";

    for (uint8_t index = 0; index < count; index++)
    {
        if (index > 0)
        {
            json += ",";
        }

        json += "{";


        json += "\"ssid\":\"";
        json += jsonEscape(networks[index].ssid);
        json += "\",";

        /*
         * Nunca enviar la contraseña al navegador.
         */
        json += "\"password_set\":";
        json += networks[index].password.isEmpty() ? "false" : "true";
        json += ",";

        json += "\"connected\":";
        json += (connected && networks[index].ssid == currentSsid) ? "true" : "false";

        json += "}";
    }

    json += "]}";

    sendJson(200, json);
}

// -------------------------------------------------------------------------------------------------
// API - GUARDAR RED WI-FI
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiSaveWiFi()
{
    if (!requireApiAuth())
    {
        return;
    }

    const String ssid = server.arg("ssid");

    String password = server.arg("password");

    if (ssid.isEmpty())
    {
        sendJson(400, R"({"ok":false,"error":"Debe seleccionar una red"})");
        return;
    }

    // =============================================================================================
    // CONSERVAR CONTRASEÑA EXISTENTE
    // =============================================================================================

    /*
     * Si el usuario está editando una red guardada y deja la contraseña
     * vacía, conservamos la contraseña almacenada anteriormente.
     */
    if (password.isEmpty())
    {
        WiFiNetwork savedNetworks[Config::MAX_WIFI_NETWORKS];

        const uint8_t savedCount =
            settingsManager->loadWiFiNetworks(
                savedNetworks,
                Config::MAX_WIFI_NETWORKS
            );

        for (uint8_t index = 0; index < savedCount; index++)
        {
            if (savedNetworks[index].ssid == ssid)
            {
                password = savedNetworks[index].password;
                break;
            }
        }
    }

    // =============================================================================================
    // PREPARAR RED
    // =============================================================================================

    WiFiNetwork network;

    network.ssid = ssid;

    network.password = password;

    // =============================================================================================
    // PAUSAR DNS
    // =============================================================================================

    const bool restartDns = hotspotManager->isDnsRunning();

    if (restartDns)
    {
        hotspotManager->stopDns();
    }


    // =============================================================================================
    // INTENTAR CONEXIÓN
    // =============================================================================================

    const bool connected = networkManager->connect(network.ssid.c_str(), network.password.c_str());

    // =============================================================================================
    // RESTAURAR DNS
    // =============================================================================================

    if (restartDns)
    {
        hotspotManager->startDns();
    }

    // =============================================================================================
    // COMPROBAR CONEXIÓN
    // =============================================================================================

    if (!connected)
    {
        sendJson(
            502,
            R"({"ok":false,"error":"No fue posible conectar. Verifique SSID y contraseña."})"
        );

        return;
    }


    // =============================================================================================
    // GUARDAR RED
    // =============================================================================================

    if (!settingsManager->saveWiFiNetwork(network))
    {
        sendJson(
            500,
            R"({"ok":false,"error":"La conexión fue establecida, pero no fue posible guardar la red."})"
        );

        return;
    }

    String json = "{\"ok\":true,\"ip\":\"";

    json += WiFi.localIP().toString();

    json += "\"}";

    sendJson(200, json);
}

// -------------------------------------------------------------------------------------------------
// API - ELIMINAR RED WI-FI
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiDeleteWiFi()
{
    if (!requireApiAuth())
    {
        return;
    }

    const String ssid = server.arg("ssid");

    if (ssid.isEmpty())
    {
        sendJson(400, R"({"ok":false,"error":"SSID no válido"})");

        return;
    }

    if (!settingsManager->deleteWiFiNetwork(ssid))
    {
        sendJson(
            404,
            R"({"ok":false,"error":"La red no está almacenada"})"
        );

        return;
    }

    /*
     * Si eliminamos precisamente la red que está conectada,
     * desconectamos también la interfaz STA.
     */
    if (networkManager->isConnected() && WiFi.SSID() == ssid)
    {
        networkManager->disconnect();
    }

    sendJson(200, R"({"ok":true})");
}


// -------------------------------------------------------------------------------------------------
// API - OBTENER CONFIGURACIÓN DE DETECCIÓN
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiDetectionSettings()
{
    if (!requireApiAuth())
    {
        return;
    }

    const DetectionSettings settings = settingsManager->loadDetectionSettings();

    String json;

    json.reserve(640);

    json += "{\"ok\":true,\"settings\":{";


    json += "\"baseline_learning_ms\":";
    json += String(settings.baselineLearningMs);
    json += ",";


    json += "\"min_baseline_samples\":";
    json += String(settings.minBaselineSamples);
    json += ",";

    json += "\"baseline_alpha\":";
    json += String(settings.baselineAlpha, 4);
    json += ",";

    json += "\"pm25_delta_threshold\":";
    json += String(settings.pm25DeltaThreshold, 2);
    json += ",";

    json += "\"fine_particle_ratio_threshold\":";
    json += String(settings.fineParticleRatioThreshold, 3);
    json += ",";

    json += "\"voc_delta_threshold\":";
    json += String(settings.vocDeltaThreshold, 2);
    json += ",";

    json += "\"nox_delta_threshold\":";
    json += String(settings.noxDeltaThreshold, 2);
    json += ",";

    json += "\"persistence_samples\":";
    json += String(settings.persistenceSamples);
    json += ",";

    json += "\"score_particles\":";
    json += String(settings.scoreParticles);
    json += ",";

    json += "\"score_fine_particles\":";
    json += String(settings.scoreFineParticles);
    json += ",";

    json += "\"score_voc\":";
    json += String(settings.scoreVoc);
    json += ",";

    json += "\"score_nox\":";
    json += String(settings.scoreNox);
    json += ",";

    json += "\"score_persistence\":";
    json += String(settings.scorePersistence);
    json += ",";

    json += "\"score_suspicious\":";
    json += String(settings.scoreSuspicious);
    json += ",";

    json += "\"score_probable\":";
    json += String(settings.scoreProbable);
    json += ",";

    json += "\"score_high_confidence\":";
    json += String(settings.scoreHighConfidence);

    json += "}}";

    sendJson(200, json);
}


// -------------------------------------------------------------------------------------------------
// API - GUARDAR CONFIGURACIÓN DE DETECCIÓN
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiSaveDetectionSettings()
{
    if (!requireApiAuth())
    {
        return;
    }

    /*
     * Partimos desde la configuración actual.
     *
     * De esta manera el endpoint admite actualizaciones parciales.
     */
    DetectionSettings settings = settingsManager->loadDetectionSettings();

    uint32_t unsignedValue = 0;

    float floatValue = 0.0f;

    // =============================================================================================
    // LÍNEA BASE
    // =============================================================================================

    if (server.hasArg("baseline_learning_ms"))
    {
        if (!parseUnsignedInteger(server.arg("baseline_learning_ms"), unsignedValue))
        {
            sendJson(
                400,
                R"({"ok":false,"error":"baseline_learning_ms no es válido"})"
            );

            return;
        }

        settings.baselineLearningMs = unsignedValue;
    }

    if (server.hasArg("min_baseline_samples"))
    {
        if (
            !parseUnsignedInteger(
                server.arg("min_baseline_samples"),
                unsignedValue
            ) ||
            unsignedValue > UINT16_MAX
        )
        {
            sendJson(
                400,
                R"({"ok":false,"error":"min_baseline_samples no es válido"})"
            );

            return;
        }

        settings.minBaselineSamples = static_cast<uint16_t>(unsignedValue);
    }

    if (server.hasArg("baseline_alpha"))
    {
        if (!parseFloatValue(server.arg("baseline_alpha"), floatValue))
        {
            sendJson(
                400,
                R"({"ok":false,"error":"baseline_alpha no es válido"})"
            );

            return;
        }

        settings.baselineAlpha = floatValue;
    }


    // =============================================================================================
    // UMBRALES
    // =============================================================================================

    if (server.hasArg("pm25_delta_threshold" ))
    {
        if (!parseFloatValue(server.arg("pm25_delta_threshold"),floatValue))
        {
            sendJson(
                400,
                R"({"ok":false,"error":"pm25_delta_threshold no es válido"})"
            );

            return;
        }

        settings.pm25DeltaThreshold = floatValue;
    }

    if (server.hasArg("fine_particle_ratio_threshold"))
    {
        if (!parseFloatValue(server.arg("fine_particle_ratio_threshold"), floatValue))
        {
            sendJson(
                400,
                R"({"ok":false,"error":"fine_particle_ratio_threshold no es válido"})"
            );

            return;
        }

        settings.fineParticleRatioThreshold = floatValue;
    }

    if (server.hasArg("voc_delta_threshold"))
    {
        if (!parseFloatValue(server.arg("voc_delta_threshold"), floatValue))
        {
            sendJson(
                400,
                R"({"ok":false,"error":"voc_delta_threshold no es válido"})"
            );

            return;
        }

        settings.vocDeltaThreshold = floatValue;
    }

    if (server.hasArg("nox_delta_threshold"))
    {
        if (!parseFloatValue(server.arg("nox_delta_threshold"),floatValue))
        {
            sendJson(
                400,
                R"({"ok":false,"error":"nox_delta_threshold no es válido"})"
            );

            return;
        }

        settings.noxDeltaThreshold = floatValue;
    }

    if (server.hasArg("persistence_samples"))
    {
        if (
            !parseUnsignedInteger(
                server.arg("persistence_samples"),
                unsignedValue
            ) ||
            unsignedValue > UINT16_MAX
        )
        {
            sendJson(
                400,
                R"({"ok":false,"error":"persistence_samples no es válido"})"
            );

            return;
        }


        settings.persistenceSamples = static_cast<uint16_t>(unsignedValue);
    }


    // =============================================================================================
    // PUNTUACIONES
    // =============================================================================================

    struct ScoreArgument
    {
        const char* name;

        uint8_t* target;
    };


    ScoreArgument scoreArguments[] =
    {
        {
            "score_particles",
            &settings.scoreParticles
        },

        {
            "score_fine_particles",
            &settings.scoreFineParticles
        },

        {
            "score_voc",
            &settings.scoreVoc
        },

        {
            "score_nox",
            &settings.scoreNox
        },

        {
            "score_persistence",
            &settings.scorePersistence
        },

        {
            "score_suspicious",
            &settings.scoreSuspicious
        },

        {
            "score_probable",
            &settings.scoreProbable
        },

        {
            "score_high_confidence",
            &settings.scoreHighConfidence
        }
    };


    for (ScoreArgument& argument : scoreArguments)
    {
        if (!server.hasArg(argument.name))
        {
            continue;
        }

        if (
            !parseUnsignedInteger(server.arg(argument.name), unsignedValue) ||
            unsignedValue > UINT8_MAX
        )
        {
            sendJson(
                400,
                String(
                    "{\"ok\":false,\"error\":\""
                ) +
                argument.name +
                " no es válido\"}"
            );

            return;
        }

        *argument.target = static_cast<uint8_t>(unsignedValue);
    }


    // =============================================================================================
    // GUARDAR
    // =============================================================================================

    if (!settingsManager->saveDetectionSettings(settings))
    {
        sendJson(
            400,
            R"({"ok":false,"error":"La configuración contiene valores no válidos"})"
        );

        return;
    }


    // =============================================================================================
    // APLICAR INMEDIATAMENTE
    // =============================================================================================

    smokeGuard->setSettings(settings);

    sendJson(200, R"({"ok":true})");
}

// -------------------------------------------------------------------------------------------------
// API - RESTAURAR CONFIGURACIÓN DE DETECCIÓN
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiResetDetectionSettings()
{
    if (!requireApiAuth())
    {
        return;
    }

    if (!settingsManager->resetDetectionSettings())
    {
        sendJson(500, R"({"ok":false,"error":"No fue posible restaurar la configuración"})");
        return;
    }

    /*
     * Al no existir valores personalizados en NVS,
     * loadDetectionSettings() devuelve los valores de fábrica.
     */
    const DetectionSettings settings = settingsManager->loadDetectionSettings();

    smokeGuard->setSettings(settings);

    sendJson(200, R"({"ok":true})");
}

// -------------------------------------------------------------------------------------------------
// API - REINICIAR LÍNEA BASE
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiResetBaseline()
{
    if (!requireApiAuth())
    {
        return;
    }

    smokeGuard->reset();


    sendJson(200, R"({"ok":true})");
}


// -------------------------------------------------------------------------------------------------
// API - REINICIAR DISPOSITIVO
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiRestart()
{
    if (!requireApiAuth())
    {
        return;
    }

    sendJson(200, R"({"ok":true,"message":"Reiniciando"})");

    delay(400);

    ESP.restart();
}


// -------------------------------------------------------------------------------------------------
// API - RESTAURAR FÁBRICA
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleApiFactoryReset()
{
    if (!requireApiAuth())
    {
        return;
    }

    bool success = true;

    success &= settingsManager->resetDetectionSettings();

    success &= settingsManager->clearWiFiNetworks();

    success &= settingsManager->resetAdminSettings();

    if (!success)
    {
        sendJson(
            500,
            R"({"ok":false,"error":"No fue posible eliminar completamente la configuración"})"
        );


        return;
    }

    clearSession();

    sendJson(200, R"({"ok":true,"message":"Configuración restaurada"})");

    delay(400);

    ESP.restart();
}

// -------------------------------------------------------------------------------------------------
// PORTAL CAUTIVO
// -------------------------------------------------------------------------------------------------

void WebConfigServer::handleCaptivePortalProbe()
{
    String location = "http://";

    location += hotspotManager->getIp().toString();

    location += "/";

    redirectTo(location);
}