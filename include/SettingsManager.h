#pragma once
// =================================================================================================
#include "DetectionSettings.h"
#include "WiFiNetwork.h"
// =================================================================================================

/**
 * @class SettingsManager
 * @file SettingsManager.h
 * @brief Administra la configuración persistente de SmokeGuard.
 *
 * Permite cargar, guardar y restaurar la configuración almacenada
 * en la memoria NVS del ESP32-S3.
 */
class SettingsManager
{
    public:

        /**
         * @brief Inicializa el administrador de configuración.
         *
         * @retval true Si fue posible acceder a NVS.
         * @retval false Si ocurrió un error.
         */
        bool begin();


        // =========================================================================================
        // CONFIGURACIÓN DE DETECCIÓN
        // =========================================================================================

        /**
         * @brief Carga la configuración de detección almacenada.
         *
         * @return Configuración de detección cargada.
         */
        DetectionSettings loadDetectionSettings();


        /**
         * @brief Guarda la configuración de detección.
         *
         * @param settings Configuración que se desea almacenar.
         *
         * @retval true Si la configuración fue guardada correctamente.
         * @retval false Si ocurrió un error.
         */
        bool saveDetectionSettings(const DetectionSettings& settings);


        /**
         * @brief Elimina la configuración personalizada de detección.
         *
         * @retval true Si la configuración fue eliminada correctamente.
         * @retval false Si ocurrió un error.
         */
        bool resetDetectionSettings();


        // =========================================================================================
        // REDES WI-FI
        // =========================================================================================

        /**
         * @brief Carga las redes Wi-Fi almacenadas.
         *
         * @param networks Arreglo donde se almacenarán las redes encontradas.
         * @param capacity Cantidad máxima de elementos disponibles en el arreglo.
         *
         * @return Cantidad de redes cargadas.
         */
        uint8_t loadWiFiNetworks(WiFiNetwork* networks, uint8_t capacity);


        /**
         * @brief Guarda o actualiza una red Wi-Fi.
         *
         * Si ya existe una red con el mismo SSID, su contraseña es
         * reemplazada. Si no existe, se agrega una nueva red.
         *
         * @param network Red que se desea guardar.
         *
         * @retval true Si la red fue guardada correctamente.
         * @retval false Si la red es inválida, no existe espacio o ocurrió un error.
         */
        bool saveWiFiNetwork(const WiFiNetwork& network);


        /**
         * @brief Elimina una red Wi-Fi almacenada.
         *
         * @param ssid Nombre de la red que se desea eliminar.
         *
         * @retval true Si la red fue eliminada correctamente.
         * @retval false Si la red no existe o ocurrió un error.
         */
        bool deleteWiFiNetwork(const String& ssid);


        /**
         * @brief Elimina todas las redes Wi-Fi almacenadas.
         *
         * @retval true Si las redes fueron eliminadas correctamente.
         * @retval false Si ocurrió un error.
         */
        bool clearWiFiNetworks();


        // =========================================================================================
        // ADMINISTRACIÓN DEL PANEL
        // =========================================================================================

        /**
         * @brief Comprueba las credenciales administrativas del panel.
         *
         * La contraseña recibida se procesa mediante PBKDF2-HMAC-SHA256
         * utilizando el salt almacenado en NVS.
         *
         * @param username Nombre de usuario recibido.
         * @param password Contraseña recibida.
         *
         * @retval true Si las credenciales son correctas.
         * @retval false Si el usuario o la contraseña son incorrectos.
         */
        bool verifyAdminCredentials(const String& username, const String& password);


        /**
         * @brief Cambia la contraseña administrativa del panel.
         *
         * Comprueba primero la contraseña actual y posteriormente genera
         * un nuevo salt y un nuevo hash para la contraseña proporcionada.
         *
         * @param currentPassword Contraseña administrativa actual.
         * @param newPassword Nueva contraseña administrativa.
         *
         * @retval true Si la contraseña fue modificada correctamente.
         * @retval false Si la contraseña actual es incorrecta o ocurrió un error.
         */
        bool changeAdminPassword(const String& currentPassword, const String& newPassword);


        /**
         * @brief Restaura las credenciales administrativas predeterminadas.
         *
         * Elimina las credenciales almacenadas y genera nuevamente el hash
         * correspondiente a la contraseña predeterminada del firmware.
         *
         * @retval true Si las credenciales fueron restauradas correctamente.
         * @retval false Si ocurrió un error.
         */
        bool resetAdminSettings();


    private:

        // =========================================================================================
        // VALIDACIÓN
        // =========================================================================================

        /**
         * @brief Comprueba que una configuración de detección sea válida.
         *
         * @param settings Configuración que se desea comprobar.
         *
         * @retval true Si la configuración es válida.
         * @retval false Si contiene valores inválidos.
         */
        bool validateDetectionSettings(const DetectionSettings& settings) const;


        /**
         * @brief Comprueba que una red Wi-Fi tenga parámetros válidos.
         *
         * @param network Red que se desea comprobar.
         *
         * @retval true Si la red es válida.
         * @retval false Si contiene parámetros inválidos.
         */
        bool validateWiFiNetwork(const WiFiNetwork& network) const;


        // =========================================================================================
        // MÉTODOS INTERNOS WI-FI
        // =========================================================================================

        /**
         * @brief Escribe el conjunto completo de redes Wi-Fi en NVS.
         *
         * @param networks Arreglo de redes.
         * @param count Cantidad de redes que deben almacenarse.
         *
         * @retval true Si las redes fueron almacenadas correctamente.
         * @retval false Si ocurrió un error.
         */
        bool writeWiFiNetworks(const WiFiNetwork* networks, uint8_t count);

        // =========================================================================================
        // ADMINISTRACIÓN DEL PANEL
        // =========================================================================================

        /**
         * @brief Comprueba que existan credenciales administrativas inicializadas.
         *
         * Si no existen, genera automáticamente el hash correspondiente
         * a las credenciales predeterminadas.
         *
         * @retval true Si las credenciales están disponibles.
         * @retval false Si ocurrió un error.
         */
        bool ensureAdminCredentials();


        /**
         * @brief Genera un hash de contraseña mediante PBKDF2-HMAC-SHA256.
         *
         * @param password Contraseña que se desea procesar.
         * @param salt Salt criptográfico.
         * @param iterations Cantidad de iteraciones PBKDF2.
         * @param output Buffer donde se almacenará el hash.
         *
         * @retval true Si el hash fue generado correctamente.
         * @retval false Si ocurrió un error criptográfico.
         */
        bool derivePasswordHash(
            const String& password,
            const uint8_t* salt,
            uint32_t iterations,
            uint8_t* output
        ) const;


        /**
         * @brief Compara dos bloques binarios evitando terminar anticipadamente.
         *
         * @param first Primer bloque.
         * @param second Segundo bloque.
         * @param length Cantidad de bytes que deben compararse.
         *
         * @retval true Si ambos bloques son iguales.
         * @retval false Si son diferentes.
         */
        bool constantTimeEquals(const uint8_t* first, const uint8_t* second, size_t length) const;
};