#pragma once
// =================================================================================================
#include <Arduino.h>
#include <WebServer.h>
// =================================================================================================


// Declaraciones adelantadas para evitar dependencias innecesarias en el header.
class HotspotManager;
class NetworkManager;
class SettingsManager;
class SmokeGuard;


/**
 * @class WebConfigServer
 * @file WebConfigServer.h
 * @brief Administra el servidor web de configuración de SmokeGuard.
 *
 * Proporciona el panel web almacenado en LittleFS, administra las sesiones
 * de autenticación y expone los endpoints HTTP utilizados para consultar
 * y modificar la configuración del dispositivo.
 *
 * La clase coordina las operaciones realizadas por HotspotManager,
 * NetworkManager, SettingsManager y SmokeGuard, sin implementar directamente
 * la lógica interna de estos componentes.
 */
class WebConfigServer
{
    public:

        /**
         * @brief Construye el servidor web de configuración.
         */
        WebConfigServer();


        /**
         * @brief Inicializa LittleFS y el servidor HTTP.
         *
         * Registra las rutas del panel web y almacena referencias a los
         * administradores utilizados para ejecutar las operaciones solicitadas
         * desde la interfaz.
         *
         * @param hotspotManager Administrador del punto de acceso y portal cautivo.
         * @param networkManager Administrador de la conexión Wi-Fi STA.
         * @param settingsManager Administrador de la configuración persistente.
         * @param smokeGuard Sistema de análisis y detección de humo.
         *
         * @retval true Si LittleFS y el servidor HTTP fueron inicializados correctamente.
         * @retval false Si ocurrió un error durante la inicialización.
         */
        bool begin(
            HotspotManager& hotspotManager,
            NetworkManager& networkManager,
            SettingsManager& settingsManager,
            SmokeGuard& smokeGuard
        );


        /**
         * @brief Procesa las solicitudes HTTP recibidas.
         *
         * Debe ejecutarse periódicamente desde loop().
         */
        void update();


    private:

        // =========================================================================================
        // SERVIDOR
        // =========================================================================================

        /**
         * @brief Servidor HTTP utilizado por el panel de configuración.
         */
        WebServer server;


        /**
         * @brief Indica si el servidor fue inicializado correctamente.
         */
        bool running = false;


        // =========================================================================================
        // COMPONENTES EXTERNOS
        // =========================================================================================

        /**
         * @brief Administrador del punto de acceso.
         */
        HotspotManager* hotspotManager = nullptr;


        /**
         * @brief Administrador de la conexión Wi-Fi.
         */
        NetworkManager* networkManager = nullptr;


        /**
         * @brief Administrador de la configuración persistente.
         */
        SettingsManager* settingsManager = nullptr;


        /**
         * @brief Sistema de análisis SmokeGuard.
         */
        SmokeGuard* smokeGuard = nullptr;


        // =========================================================================================
        // SESIÓN
        // =========================================================================================

        /**
         * @brief Token de la sesión administrativa actualmente activa.
         */
        String sessionToken;


        /**
         * @brief Momento de la última actividad de la sesión.
         */
        uint32_t sessionLastActivityMs = 0;


        // =========================================================================================
        // CONFIGURACIÓN DEL SERVIDOR
        // =========================================================================================

        /**
         * @brief Registra todas las rutas HTTP utilizadas por el panel.
         */
        void configureRoutes();


        // =========================================================================================
        // AUTENTICACIÓN
        // =========================================================================================

        /**
         * @brief Genera un token aleatorio para una nueva sesión.
         *
         * @return Token hexadecimal de sesión.
         */
        String generateSessionToken() const;


        /**
         * @brief Comprueba si la sesión actual ha expirado.
         *
         * @retval true Si no existe sesión o superó el tiempo máximo de inactividad.
         * @retval false Si la sesión continúa siendo válida.
         */
        bool sessionExpired() const;


        /**
         * @brief Comprueba si la solicitud actual pertenece a una sesión autenticada.
         *
         * Si la sesión es válida, actualiza el momento de última actividad.
         *
         * @retval true Si la solicitud está autenticada.
         * @retval false Si no existe una sesión válida.
         */
        bool isAuthenticated();


        /**
         * @brief Exige autenticación para acceder a un endpoint API.
         *
         * Si la solicitud no está autenticada, envía automáticamente
         * una respuesta HTTP 401.
         *
         * @retval true Si la solicitud está autenticada.
         * @retval false Si se envió una respuesta de acceso denegado.
         */
        bool requireApiAuth();


        /**
         * @brief Elimina la sesión administrativa actual.
         */
        void clearSession();


        // =========================================================================================
        // UTILIDADES HTTP
        // =========================================================================================

        /**
         * @brief Envía encabezados HTTP para impedir el almacenamiento en caché.
         */
        void sendNoCacheHeaders();


        /**
         * @brief Envía una respuesta JSON.
         *
         * @param statusCode Código de estado HTTP.
         * @param json Contenido JSON que se desea enviar.
         */
        void sendJson(
            int statusCode,
            const String& json
        );


        /**
         * @brief Redirige la solicitud hacia otra ruta.
         *
         * @param location Ruta o URL de destino.
         */
        void redirectTo(
            const String& location
        );


        /**
         * @brief Determina el Content-Type correspondiente a un archivo.
         *
         * @param path Ruta del archivo.
         *
         * @return Tipo MIME correspondiente.
         */
        String contentTypeForPath(
            const String& path
        ) const;


        /**
         * @brief Envía un archivo almacenado en LittleFS.
         *
         * @param path Ruta absoluta dentro de LittleFS.
         *
         * @retval true Si el archivo fue enviado correctamente.
         * @retval false Si el archivo no existe o no pudo abrirse.
         */
        bool sendFile(
            const String& path
        );


        /**
         * @brief Escapa una cadena para poder incorporarla a JSON.
         *
         * @param value Cadena original.
         *
         * @return Cadena escapada.
         */
        String jsonEscape(
            const String& value
        ) const;


        // =========================================================================================
        // PÁGINAS
        // =========================================================================================

        /**
         * @brief Procesa la ruta raíz del servidor.
         */
        void handleRoot();


        /**
         * @brief Muestra la página de inicio de sesión.
         */
        void handleLoginPage();


        /**
         * @brief Envía una página que requiere autenticación.
         *
         * @param filePath Archivo HTML almacenado en LittleFS.
         */
        void handleProtectedPage(
            const char* filePath
        );


        /**
         * @brief Cierra la sesión administrativa.
         */
        void handleLogout();


        // =========================================================================================
        // API - AUTENTICACIÓN
        // =========================================================================================

        /**
         * @brief Procesa una solicitud de inicio de sesión.
         */
        void handleApiLogin();


        /**
         * @brief Procesa el cambio de contraseña administrativa.
         */
        void handleApiPassword();


        // =========================================================================================
        // API - ESTADO
        // =========================================================================================

        /**
         * @brief Devuelve información general del dispositivo y la red.
         */
        void handleApiStatus();


        // =========================================================================================
        // API - WI-FI
        // =========================================================================================

        /**
         * @brief Escanea las redes Wi-Fi disponibles.
         */
        void handleApiNetworks();


        /**
         * @brief Devuelve las redes Wi-Fi guardadas en el dispositivo.
         *
         * Las contraseñas nunca se incluyen en la respuesta HTTP.
         */
        void handleApiSavedNetworks();


        /**
         * @brief Guarda una red Wi-Fi e intenta establecer la conexión.
         */
        void handleApiSaveWiFi();


        /**
         * @brief Elimina una red Wi-Fi almacenada.
         */
        void handleApiDeleteWiFi();


        // =========================================================================================
        // API - DETECCIÓN
        // =========================================================================================

        /**
         * @brief Devuelve la configuración actual del algoritmo de detección.
         */
        void handleApiDetectionSettings();


        /**
         * @brief Guarda una nueva configuración del algoritmo de detección.
         *
         * Después de almacenarla, aplica inmediatamente los nuevos parámetros
         * a la instancia de SmokeGuard.
         */
        void handleApiSaveDetectionSettings();


        /**
         * @brief Restaura los valores predeterminados de detección.
         */
        void handleApiResetDetectionSettings();


        /**
         * @brief Reinicia el aprendizaje de la línea base de SmokeGuard.
         */
        void handleApiResetBaseline();


        // =========================================================================================
        // API - SISTEMA
        // =========================================================================================

        /**
         * @brief Reinicia el ESP32-S3.
         */
        void handleApiRestart();


        /**
         * @brief Elimina la configuración personalizada y reinicia el dispositivo.
         */
        void handleApiFactoryReset();


        // =========================================================================================
        // PORTAL CAUTIVO
        // =========================================================================================

        /**
         * @brief Redirige las solicitudes de detección de portal cautivo
         * hacia la página principal de SmokeGuard.
         */
        void handleCaptivePortalProbe();
};