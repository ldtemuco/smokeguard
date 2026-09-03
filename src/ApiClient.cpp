#include "ApiClient.h"
#include "Config.h"
// =================================================================================================
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <cmath>
// =================================================================================================


// -------------------------------------------------------------------------------------------------
// CERTIFICADOS TLS
// -------------------------------------------------------------------------------------------------

extern const uint8_t apiRootsPemStart[] asm("_binary_certs_api_roots_pem_start");

// -------------------------------------------------------------------------------------------------
// ENVIAR ALERTA
// -------------------------------------------------------------------------------------------------

bool ApiClient::sendAlert(const AlertData& alert)
{
    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("           ENVIANDO ALERTA");
    Serial0.println("========================================");


    // ---------------------------------------------------------------------------------------------
    // VALIDAR ALERTA
    // ---------------------------------------------------------------------------------------------

    if (!validateAlert(alert))
    {
        Serial0.println("[ERROR] Los datos de la alerta no son válidos.");
        return false;
    }


    // ---------------------------------------------------------------------------------------------
    // COMPROBAR CONEXIÓN WI-FI
    // ---------------------------------------------------------------------------------------------

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial0.println("[ERROR] No existe una conexión Wi-Fi activa.");
        return false;
    }


    // ---------------------------------------------------------------------------------------------
    // CLIENTE HTTPS
    // ---------------------------------------------------------------------------------------------

    WiFiClientSecure client;

    /*
     * Utilizar las autoridades certificadoras almacenadas en api_roots.pem.
     *
     * A diferencia de setInsecure(), esto obliga al cliente TLS a validar
     * el certificado presentado por el servidor.
     */
    client.setCACert(reinterpret_cast<const char*>(apiRootsPemStart));

    // Tiempo máximo para completar el handshake TLS.
    client.setHandshakeTimeout(Config::API_TLS_HANDSHAKE_TIMEOUT_S);


    // ---------------------------------------------------------------------------------------------
    // CLIENTE HTTP
    // ---------------------------------------------------------------------------------------------

    HTTPClient http;

    // Tiempo máximo para establecer la conexión TCP.
    http.setConnectTimeout(Config::API_CONNECT_TIMEOUT_MS);

    // Tiempo máximo para recibir una respuesta HTTP.
    http.setTimeout(Config::API_RESPONSE_TIMEOUT_MS);


    Serial0.println("[HTTPS] Preparando conexión segura...");

    if (!http.begin(client, Config::API_ALERTS_URL))
    {
        Serial0.println("[ERROR] No fue posible iniciar la conexión HTTPS.");
        return false;
    }

    // ---------------------------------------------------------------------------------------------
    // ENCABEZADOS HTTP
    // ---------------------------------------------------------------------------------------------

    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "application/json");
    http.addHeader("User-Agent", "SmokeGuard/1.0");

    // ---------------------------------------------------------------------------------------------
    // CREAR JSON
    // ---------------------------------------------------------------------------------------------

    const String body = createAlertJson(alert);

    Serial0.println();
    Serial0.println("[HTTP] JSON:");
    Serial0.println(body);

    // ---------------------------------------------------------------------------------------------
    // ENVIAR SOLICITUD
    // ---------------------------------------------------------------------------------------------

    Serial0.println();
    Serial0.println("[HTTP] Ejecutando POST...");

    const uint32_t startTime = millis();

    const int responseCode = http.POST(body);

    const uint32_t elapsedTime = millis() - startTime;

    Serial0.println("[HTTP] POST finalizado.");

    Serial0.printf("[HTTP] Tiempo: %lu ms\n", elapsedTime);

    Serial0.printf("[HTTP] Código: %d\n", responseCode);

    // ---------------------------------------------------------------------------------------------
    // COMPROBAR ERROR DE COMUNICACIÓN
    // ---------------------------------------------------------------------------------------------

    if (responseCode <= 0)
    {
        Serial0.printf("[ERROR] %s\n", http.errorToString(responseCode).c_str());
        http.end();
        return false;
    }

    // ---------------------------------------------------------------------------------------------
    // LEER RESPUESTA
    // ---------------------------------------------------------------------------------------------

    const String response = http.getString();

    Serial0.println("[HTTP] Respuesta:");

    if (response.length() > 0)
    {
        Serial0.println(response);
    }
    else
    {
        Serial0.println("[VACÍA]");
    }

    // Cerrar la conexión HTTP.
    http.end();

    // ---------------------------------------------------------------------------------------------
    // COMPROBAR RESPUESTA HTTP
    // ---------------------------------------------------------------------------------------------

    if (responseCode >= 200 && responseCode < 300)
    {
        Serial0.println();
        Serial0.println("[OK] Alerta enviada correctamente.");
        return true;
    }

    Serial0.println();
    Serial0.printf("[ERROR] API rechazó la solicitud. Código HTTP: %d\n", responseCode);

    return false;
}


// -------------------------------------------------------------------------------------------------
// VALIDAR ALERTA
// -------------------------------------------------------------------------------------------------

bool ApiClient::validateAlert(const AlertData& alert) const
{
    if (alert.deviceId.length() == 0)
    {
        return false;
    }

    if (alert.sensor.length() == 0)
    {
        return false;
    }

    if (alert.alertType.length() == 0)
    {
        return false;
    }

    if (!std::isfinite(alert.value))
    {
        return false;
    }

    if (alert.unit.length() == 0)
    {
        return false;
    }

    if (alert.level.length() == 0)
    {
        return false;
    }

    return true;
}


// -------------------------------------------------------------------------------------------------
// CREAR JSON DE ALERTA
// -------------------------------------------------------------------------------------------------

String ApiClient::createAlertJson(const AlertData& alert) const
{
    String json;

    // Reservar memoria para reducir reasignaciones internas.
    json.reserve(256);

    json += "{";

    json += "\"device_id\":\"";
    json += escapeJsonString(alert.deviceId);
    json += "\",";

    json += "\"sensor\":\"";
    json += escapeJsonString(alert.sensor);
    json += "\",";

    json += "\"alert_type\":\"";
    json += escapeJsonString(alert.alertType);
    json += "\",";

    json += "\"value\":";
    json += String(alert.value, 2);
    json += ",";

    json += "\"unit\":\"";
    json += escapeJsonString(alert.unit);
    json += "\",";

    json += "\"level\":\"";
    json += escapeJsonString(alert.level);
    json += "\"";

    json += "}";

    return json;
}


// -------------------------------------------------------------------------------------------------
// ESCAPAR CADENA JSON
// -------------------------------------------------------------------------------------------------

String ApiClient::escapeJsonString(const String& value) const
{
    String result;

    result.reserve(value.length() + 8);


    for (size_t index = 0; index < value.length(); index++)
    {
        const char character = value[index];

        switch (character)
        {
            case '"':
                result += "\\\"";
                break;

            case '\\':
                result += "\\\\";
                break;

            case '\b':
                result += "\\b";
                break;

            case '\f':
                result += "\\f";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            default:
                result += character;
                break;
        }
    }

    return result;
}