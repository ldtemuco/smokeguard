#include "HotspotManager.h"
#include "Config.h"
// =================================================================================================


// -------------------------------------------------------------------------------------------------
// INICIAR HOTSPOT
// -------------------------------------------------------------------------------------------------

bool HotspotManager::begin()
{
    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("          HOTSPOT SMOKEGUARD");
    Serial0.println("========================================");

    // ---------------------------------------------------------------------------------------------
    // CONFIGURAR MODO WI-FI
    // ---------------------------------------------------------------------------------------------

    /*
     * SmokeGuard utiliza simultáneamente:
     *
     * AP  -> Panel local de configuración.
     * STA -> Conexión a una red Wi-Fi externa.
     */
    WiFi.mode(WIFI_AP_STA);

    // ---------------------------------------------------------------------------------------------
    // CONFIGURAR DIRECCIÓN IP DEL AP
    // ---------------------------------------------------------------------------------------------

    if (!WiFi.softAPConfig(apIp, apGateway, apSubnet))
    {
        Serial0.println("[AP] ERROR: no fue posible configurar la dirección IP.");
        return false;
    }

    // ---------------------------------------------------------------------------------------------
    // INICIAR PUNTO DE ACCESO
    // ---------------------------------------------------------------------------------------------

    if (!WiFi.softAP(Config::AP_SSID, Config::AP_PASSWORD))
    {
        Serial0.println("[AP] ERROR: no fue posible iniciar el punto de acceso.");
        return false;
    }

    // ---------------------------------------------------------------------------------------------
    // MOSTRAR INFORMACIÓN
    // ---------------------------------------------------------------------------------------------

    Serial0.println("[OK] Punto de acceso iniciado.");

    Serial0.printf("[AP] SSID: %s\n", Config::AP_SSID);

    Serial0.printf("[AP] IP: %s\n", WiFi.softAPIP().toString().c_str());

    // ---------------------------------------------------------------------------------------------
    // INICIAR DNS
    // ---------------------------------------------------------------------------------------------

    if (!startDns())
    {
        /*
         * El punto de acceso puede seguir funcionando aunque el DNS
         * del portal cautivo no haya podido iniciarse.
         *
         * Por este motivo no detenemos el AP.
         */
        Serial0.println("[AP] ADVERTENCIA: el portal cautivo no está disponible.");
    }

    return true;
}


// -------------------------------------------------------------------------------------------------
// ACTUALIZAR HOTSPOT
// -------------------------------------------------------------------------------------------------

void HotspotManager::update()
{
    /*
     * DNSServer necesita procesar periódicamente las solicitudes
     * recibidas.
     */
    if (dnsServerRunning)
    {
        dnsServer.processNextRequest();
    }
}


// -------------------------------------------------------------------------------------------------
// INICIAR SERVIDOR DNS
// -------------------------------------------------------------------------------------------------

bool HotspotManager::startDns()
{
    // ---------------------------------------------------------------------------------------------
    // DETENER INSTANCIA ANTERIOR
    // ---------------------------------------------------------------------------------------------

    /*
     * Reiniciar siempre el servidor DNS evita conservar un socket UDP
     * inválido después de modificaciones en la interfaz STA.
     */
    stopDns();


    // ---------------------------------------------------------------------------------------------
    // COMPROBAR AP
    // ---------------------------------------------------------------------------------------------

    const IPAddress currentApIp = WiFi.softAPIP();

    if (currentApIp == IPAddress(0, 0, 0, 0))
    {
        Serial0.println("[DNS] ERROR: el punto de acceso no tiene una dirección IP válida.");
        return false;
    }

    // ---------------------------------------------------------------------------------------------
    // INICIAR DNS DEL PORTAL CAUTIVO
    // ---------------------------------------------------------------------------------------------

    /*
     * El comodín "*" provoca que cualquier nombre de dominio solicitado
     * sea resuelto hacia la dirección IP del punto de acceso.
     */
    if (!dnsServer.start(Config::DNS_PORT, "*", currentApIp))
    {
        dnsServerRunning = false;
        Serial0.println("[DNS] ERROR: no fue posible iniciar el servidor DNS.");
        return false;
    }

    dnsServerRunning = true;

    Serial0.printf(
        "[DNS] Portal cautivo activo en %s:%u\n", 
        currentApIp.toString().c_str(),
        Config::DNS_PORT
    );


    return true;
}

// -------------------------------------------------------------------------------------------------
// DETENER SERVIDOR DNS
// -------------------------------------------------------------------------------------------------

void HotspotManager::stopDns()
{
    if (!dnsServerRunning)
    {
        return;
    }

    dnsServer.stop();

    dnsServerRunning = false;

    /*
     * Pequeña pausa para permitir que el socket UDP utilizado
     * internamente por DNSServer sea liberado completamente.
     */
    delay(10);
    
    Serial0.println("[DNS] Servidor detenido.");
}


// -------------------------------------------------------------------------------------------------
// COMPROBAR ESTADO DEL SERVIDOR DNS
// -------------------------------------------------------------------------------------------------

bool HotspotManager::isDnsRunning() const
{
    return dnsServerRunning;
}


// -------------------------------------------------------------------------------------------------
// OBTENER DIRECCIÓN IP DEL HOTSPOT
// -------------------------------------------------------------------------------------------------

IPAddress HotspotManager::getIp() const
{
    return WiFi.softAPIP();
}