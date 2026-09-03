#include "SettingsManager.h"
#include "CryptoManager.h"
// =================================================================================================
#include <Preferences.h>
#include <esp_random.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <cmath>
// =================================================================================================
namespace
{
    // =============================================================================================
    // NAMESPACE WI-FI
    // =============================================================================================

    constexpr char WIFI_NAMESPACE[] = "wifi";

    // =============================================================================================
    // CLAVES WI-FI
    // =============================================================================================

    constexpr char KEY_WIFI_COUNT[] = "count";

    // ---------------------------------------------------------------------------------------------
    // GENERAR CLAVE SSID
    // ---------------------------------------------------------------------------------------------

    String makeWiFiSsidKey(uint8_t index)
    {
        return "ssid" + String(index);
    }

    // ---------------------------------------------------------------------------------------------
    // GENERAR CLAVE CONTRASEÑA
    // ---------------------------------------------------------------------------------------------

    String makeWiFiPasswordKey(uint8_t index)
    {
        return "pass" + String(index);
    }

    // =============================================================================================
    // NAMESPACE NVS
    // =============================================================================================

    constexpr char DETECTION_NAMESPACE[] = "detection";

    // =============================================================================================
    // CLAVES DE LÍNEA BASE
    // =============================================================================================

    constexpr char KEY_BASELINE_LEARNING_MS[] = "base_ms";

    constexpr char KEY_MIN_BASELINE_SAMPLES[] = "base_samples";

    constexpr char KEY_BASELINE_ALPHA[] = "base_alpha";


    // =============================================================================================
    // CLAVES DE UMBRALES
    // =============================================================================================

    constexpr char KEY_PM25_DELTA[] = "pm25_delta";

    constexpr char KEY_FINE_PARTICLE_RATIO[] = "fine_ratio";

    constexpr char KEY_VOC_DELTA[] = "voc_delta";

    constexpr char KEY_NOX_DELTA[] = "nox_delta";

    constexpr char KEY_PERSISTENCE_SAMPLES[] = "persist";


    // =============================================================================================
    // CLAVES DE PUNTUACIONES
    // =============================================================================================

    constexpr char KEY_SCORE_PARTICLES[] = "pts_pm";

    constexpr char KEY_SCORE_FINE_PARTICLES[] = "pts_fine";

    constexpr char KEY_SCORE_VOC[] = "pts_voc";

    constexpr char KEY_SCORE_NOX[] = "pts_nox";

    constexpr char KEY_SCORE_PERSISTENCE[] = "pts_persist";


    // =============================================================================================
    // CLAVES DE ESTADOS
    // =============================================================================================

    constexpr char KEY_SCORE_SUSPICIOUS[] = "st_susp";

    constexpr char KEY_SCORE_PROBABLE[] = "st_prob";

    constexpr char KEY_SCORE_HIGH_CONFIDENCE[] = "st_high";

    // =============================================================================================
    // CONFIGURACIÓN ADMINISTRATIVA
    // =============================================================================================

    constexpr char PANEL_NAMESPACE[] = "panel";

    constexpr char KEY_ADMIN_USER[] = "admin_user";

    constexpr char KEY_ADMIN_SALT[] = "admin_salt";

    constexpr char KEY_ADMIN_HASH[] = "admin_hash";

    constexpr char KEY_ADMIN_ITERATIONS[] = "admin_iter";

    // =============================================================================================
    // PARÁMETROS DE CONTRASEÑA
    // =============================================================================================

    constexpr size_t ADMIN_SALT_SIZE = 16;

    constexpr size_t ADMIN_HASH_SIZE = 32;

    // =============================================================================================
    // CONVERTIR BYTE A HEXADECIMAL
    // =============================================================================================

    char nibbleToHex(uint8_t value)
    {
        return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('a' + value - 10);
    }


    // =============================================================================================
    // CONVERTIR HEXADECIMAL A BYTE
    // =============================================================================================

    int8_t hexToNibble(char character)
    {
        if (character >= '0' && character <= '9')
        {
            return character - '0';
        }

        if (character >= 'a' && character <= 'f')
        {
            return character - 'a' + 10;
        }

        if (character >= 'A' && character <= 'F')
        {
            return character - 'A' + 10;
        }

        return -1;
    }


    // =============================================================================================
    // CONVERTIR BYTES A HEXADECIMAL
    // =============================================================================================

    String bytesToHex(const uint8_t* data, size_t length)
    {
        String result;

        result.reserve(length * 2);

        for (size_t index = 0; index < length; index++)
        {
            result += nibbleToHex(data[index] >> 4);
            result += nibbleToHex(data[index] & 0x0F);
        }

        return result;
    }


    // =============================================================================================
    // CONVERTIR HEXADECIMAL A BYTES
    // =============================================================================================

    bool hexToBytes(const String& value, uint8_t* output, size_t outputLength)
    {
        if (output == nullptr || value.length() != outputLength * 2)
        {
            return false;
        }

        for (size_t index = 0; index < outputLength; index++)
        {
            const int8_t high = hexToNibble(value[index * 2]);

            const int8_t low = hexToNibble(value[index * 2 + 1]);

            if (high < 0 || low < 0)
            {
                return false;
            }

            output[index] = static_cast<uint8_t>((high << 4) | low);
        }

        return true;
    }
}


// =================================================================================================
// INICIAR ADMINISTRADOR DE CONFIGURACIÓN
// =================================================================================================

bool SettingsManager::begin()
{
    // =============================================================================================
    // COMPROBAR CONFIGURACIÓN DE DETECCIÓN
    // =============================================================================================

    Preferences preferences;

    if (!preferences.begin(DETECTION_NAMESPACE, false))
    {
        Serial0.println("[Settings] ERROR: no fue posible acceder a la configuración.");
        return false;
    }

    preferences.end();


    // =============================================================================================
    // INICIALIZAR CREDENCIALES ADMINISTRATIVAS
    // =============================================================================================

    if (!ensureAdminCredentials())
    {
        Serial0.println(
            "[Settings] ERROR: no fue posible inicializar las credenciales administrativas."
        );

        return false;
    }

    Serial0.println("[Settings] Configuración disponible.");

    return true;
}



// -------------------------------------------------------------------------------------------------
// GENERAR HASH DE CONTRASEÑA
// -------------------------------------------------------------------------------------------------

bool SettingsManager::derivePasswordHash(
    const String& password,
    const uint8_t* salt,
    uint32_t iterations,
    uint8_t* output
) const
{
    if (salt == nullptr || output == nullptr || iterations == 0)
    {
        return false;
    }


    // =============================================================================================
    // OBTENER SHA-256
    // =============================================================================================

    const mbedtls_md_info_t* mdInfo =
        mbedtls_md_info_from_type(
            MBEDTLS_MD_SHA256
        );


    if (mdInfo == nullptr)
    {
        Serial0.println("[Settings] ERROR: SHA-256 no disponible.");
        return false;
    }


    // =============================================================================================
    // PREPARAR CONTEXTO HMAC
    // =============================================================================================

    mbedtls_md_context_t context;


    mbedtls_md_init(
        &context
    );


    const int setupResult =
        mbedtls_md_setup(
            &context,
            mdInfo,
            1
        );


    if (setupResult != 0)
    {
        Serial0.printf(
            "[Settings] ERROR: inicialización PBKDF2 fallida. Código: %d\n",
            setupResult
        );


        mbedtls_md_free(
            &context
        );


        return false;
    }


    // =============================================================================================
    // EJECUTAR PBKDF2-HMAC-SHA256
    // =============================================================================================

    const int result =
        mbedtls_pkcs5_pbkdf2_hmac(
            &context,
            reinterpret_cast<const uint8_t*>(
                password.c_str()
            ),
            password.length(),
            salt,
            ADMIN_SALT_SIZE,
            iterations,
            ADMIN_HASH_SIZE,
            output
        );


    mbedtls_md_free(&context);


    if (result != 0)
    {
        Serial0.printf("[Settings] ERROR: PBKDF2 falló. Código: %d\n", result);

        return false;
    }

    return true;
}


// -------------------------------------------------------------------------------------------------
// COMPARAR BLOQUES EN TIEMPO CONSTANTE
// -------------------------------------------------------------------------------------------------

bool SettingsManager::constantTimeEquals(
    const uint8_t* first,
    const uint8_t* second,
    size_t length
) const
{
    if (first == nullptr || second == nullptr)
    {
        return false;
    }

    uint8_t difference = 0;


    for (size_t index = 0; index < length; index++)
    {
        difference |= first[index] ^ second[index];
    }


    return difference == 0;
}

// -------------------------------------------------------------------------------------------------
// GARANTIZAR CREDENCIALES ADMINISTRATIVAS
// -------------------------------------------------------------------------------------------------

bool SettingsManager::ensureAdminCredentials()
{
    Preferences preferences;


    if (!preferences.begin(PANEL_NAMESPACE, false))
    {
        return false;
    }


    // =============================================================================================
    // COMPROBAR CREDENCIALES EXISTENTES
    // =============================================================================================

    const bool credentialsExist =
        preferences.isKey(KEY_ADMIN_USER) &&
        preferences.isKey(KEY_ADMIN_SALT) &&
        preferences.isKey(KEY_ADMIN_HASH) &&
        preferences.isKey(KEY_ADMIN_ITERATIONS);


    if (credentialsExist)
    {
        preferences.end();

        return true;
    }

    Serial0.println("[Settings] Creando credenciales administrativas iniciales.");


    // =============================================================================================
    // GENERAR SALT
    // =============================================================================================

    uint8_t salt[ADMIN_SALT_SIZE]{};

    esp_fill_random(salt, sizeof(salt));

    // =============================================================================================
    // GENERAR HASH
    // =============================================================================================

    uint8_t hash[ADMIN_HASH_SIZE]{};

    if (
        !derivePasswordHash(
            Config::DEFAULT_ADMIN_PASSWORD,
            salt,
            Config::ADMIN_PBKDF2_ITERATIONS,
            hash
        )
    )
    {
        preferences.end();
        return false;
    }

    const String saltHex = bytesToHex(salt, sizeof(salt));

    const String hashHex = bytesToHex(hash, sizeof(hash));

    // =============================================================================================
    // GUARDAR
    // =============================================================================================

    bool success = true;

    success &=
        preferences.putString(
            KEY_ADMIN_USER,
            Config::DEFAULT_ADMIN_USER
        ) > 0;


    success &=
        preferences.putString(
            KEY_ADMIN_SALT,
            saltHex
        ) > 0;


    success &=
        preferences.putString(
            KEY_ADMIN_HASH,
            hashHex
        ) > 0;


    success &=
        preferences.putUInt(
            KEY_ADMIN_ITERATIONS,
            Config::ADMIN_PBKDF2_ITERATIONS
        ) > 0;

    preferences.end();

    if (!success)
    {
        Serial0.println(
            "[Settings] ERROR: no fue posible guardar las credenciales administrativas."
        );

        return false;
    }

    Serial0.println("[Settings] Credenciales administrativas inicializadas.");

    return true;
}

// -------------------------------------------------------------------------------------------------
// VERIFICAR CREDENCIALES ADMINISTRATIVAS
// -------------------------------------------------------------------------------------------------

bool SettingsManager::verifyAdminCredentials(const String& username, const String& password)
{
    if (username.isEmpty() || password.isEmpty())
    {
        return false;
    }


    Preferences preferences;


    if (!preferences.begin(PANEL_NAMESPACE, true))
    {
        return false;
    }


    // =============================================================================================
    // CARGAR CREDENCIALES
    // =============================================================================================

    const String storedUsername =
        preferences.getString(
            KEY_ADMIN_USER,
            ""
        );


    const String saltHex =
        preferences.getString(
            KEY_ADMIN_SALT,
            ""
        );


    const String hashHex =
        preferences.getString(
            KEY_ADMIN_HASH,
            ""
        );


    const uint32_t iterations =
        preferences.getUInt(
            KEY_ADMIN_ITERATIONS,
            0
        );

    preferences.end();


    // =============================================================================================
    // COMPROBAR USUARIO
    // =============================================================================================

    if (username != storedUsername)
    {
        return false;
    }


    // =============================================================================================
    // RECUPERAR SALT Y HASH
    // =============================================================================================

    uint8_t salt[ADMIN_SALT_SIZE]{};


    uint8_t storedHash[
        ADMIN_HASH_SIZE
    ]{};


    if (
        !hexToBytes(
            saltHex,
            salt,
            sizeof(salt)
        ) ||
        !hexToBytes(
            hashHex,
            storedHash,
            sizeof(storedHash)
        ) ||
        iterations == 0
    )
    {
        Serial0.println("[Settings] ERROR: credenciales administrativas almacenadas no válidas.");

        return false;
    }


    // =============================================================================================
    // GENERAR HASH DE LA CONTRASEÑA RECIBIDA
    // =============================================================================================

    uint8_t candidateHash[ADMIN_HASH_SIZE]{};


    if (!derivePasswordHash(password, salt, iterations, candidateHash))
    {
        return false;
    }


    // =============================================================================================
    // COMPARAR
    // =============================================================================================

    return constantTimeEquals(storedHash, candidateHash, sizeof(storedHash));
}

// -------------------------------------------------------------------------------------------------
// CAMBIAR CONTRASEÑA ADMINISTRATIVA
// -------------------------------------------------------------------------------------------------

bool SettingsManager::changeAdminPassword(const String& currentPassword, const String& newPassword)
{
    if (newPassword.length() < 8)
    {
        return false;
    }

    // =============================================================================================
    // OBTENER USUARIO ACTUAL
    // =============================================================================================

    Preferences preferences;

    if (!preferences.begin(PANEL_NAMESPACE,true))
    {
        return false;
    }

    const String username = preferences.getString(KEY_ADMIN_USER, Config::DEFAULT_ADMIN_USER);

    preferences.end();

    // =============================================================================================
    // COMPROBAR CONTRASEÑA ACTUAL
    // =============================================================================================

    if (!verifyAdminCredentials(username, currentPassword))
    {
        return false;
    }

    // =============================================================================================
    // GENERAR NUEVO SALT
    // =============================================================================================

    uint8_t salt[ADMIN_SALT_SIZE]{};

    esp_fill_random(salt, sizeof(salt));

    // =============================================================================================
    // GENERAR NUEVO HASH
    // =============================================================================================

    uint8_t hash[ADMIN_HASH_SIZE]{};

    if (!derivePasswordHash(newPassword, salt, Config::ADMIN_PBKDF2_ITERATIONS, hash))
    {
        return false;
    }

    const String saltHex = bytesToHex(salt, sizeof(salt));

    const String hashHex = bytesToHex(hash,sizeof(hash));

    // =============================================================================================
    // GUARDAR
    // =============================================================================================

    if (!preferences.begin(PANEL_NAMESPACE, false))
    {
        return false;
    }

    bool success = true;


    success &= preferences.putString(KEY_ADMIN_SALT, saltHex) > 0;

    success &= preferences.putString(KEY_ADMIN_HASH, hashHex) > 0;

    success &= preferences.putUInt(KEY_ADMIN_ITERATIONS, Config::ADMIN_PBKDF2_ITERATIONS) > 0;

    preferences.end();

    if (!success)
    {
        Serial0.println("[Settings] ERROR: no fue posible cambiar la contraseña administrativa.");
        return false;
    }

    Serial0.println("[Settings] Contraseña administrativa actualizada.");

    return true;
}

// -------------------------------------------------------------------------------------------------
// RESTAURAR CREDENCIALES ADMINISTRATIVAS
// -------------------------------------------------------------------------------------------------

bool SettingsManager::resetAdminSettings()
{
    Preferences preferences;

    if (!preferences.begin(PANEL_NAMESPACE, false))
    {
        return false;
    }

    const bool success = preferences.clear();

    preferences.end();

    if (!success)
    {
        Serial0.println(
            "[Settings] ERROR: no fue posible eliminar las credenciales administrativas."
        );

        return false;
    }


    /*
     * Crear inmediatamente las credenciales predeterminadas.
     *
     * Se genera un nuevo salt, por lo que incluso la contraseña de
     * fábrica producirá un hash diferente al utilizado anteriormente.
     */
    if (!ensureAdminCredentials())
    {
        return false;
    }

    Serial0.println("[Settings] Credenciales administrativas restauradas.");

    return true;
}


// =================================================================================================
// CARGAR CONFIGURACIÓN DE DETECCIÓN
// =================================================================================================

DetectionSettings SettingsManager::loadDetectionSettings()
{
    /*
     * La estructura se construye inicialmente utilizando los valores
     * predeterminados definidos en Config.h.
     */
    DetectionSettings settings;

    Preferences preferences;

    if (!preferences.begin(DETECTION_NAMESPACE, true))
    {
        Serial0.println("[Settings] ERROR: no fue posible leer la configuración de detección.");

        Serial0.println("[Settings] Se utilizarán los valores predeterminados.");

        return settings;
    }


    // =============================================================================================
    // LÍNEA BASE
    // =============================================================================================

    settings.baselineLearningMs =
        preferences.getULong(
            KEY_BASELINE_LEARNING_MS,
            settings.baselineLearningMs
        );

    settings.minBaselineSamples =
        preferences.getUShort(
            KEY_MIN_BASELINE_SAMPLES,
            settings.minBaselineSamples
        );

    settings.baselineAlpha =
        preferences.getFloat(
            KEY_BASELINE_ALPHA,
            settings.baselineAlpha
        );

    // =============================================================================================
    // UMBRALES
    // =============================================================================================

    settings.pm25DeltaThreshold =
        preferences.getFloat(
            KEY_PM25_DELTA,
            settings.pm25DeltaThreshold
        );

    settings.fineParticleRatioThreshold =
        preferences.getFloat(
            KEY_FINE_PARTICLE_RATIO,
            settings.fineParticleRatioThreshold
        );

    settings.vocDeltaThreshold =
        preferences.getFloat(
            KEY_VOC_DELTA,
            settings.vocDeltaThreshold
        );

    settings.noxDeltaThreshold =
        preferences.getFloat(
            KEY_NOX_DELTA,
            settings.noxDeltaThreshold
        );

    settings.persistenceSamples =
        preferences.getUShort(
            KEY_PERSISTENCE_SAMPLES,
            settings.persistenceSamples
        );

    // =============================================================================================
    // PUNTUACIONES
    // =============================================================================================

    settings.scoreParticles =
        preferences.getUChar(
            KEY_SCORE_PARTICLES,
            settings.scoreParticles
        );

    settings.scoreFineParticles =
        preferences.getUChar(
            KEY_SCORE_FINE_PARTICLES,
            settings.scoreFineParticles
        );

    settings.scoreVoc =
        preferences.getUChar(
            KEY_SCORE_VOC,
            settings.scoreVoc
        );

    settings.scoreNox =
        preferences.getUChar(
            KEY_SCORE_NOX,
            settings.scoreNox
        );

    settings.scorePersistence =
        preferences.getUChar(
            KEY_SCORE_PERSISTENCE,
            settings.scorePersistence
        );


    // =============================================================================================
    // ESTADOS
    // =============================================================================================

    settings.scoreSuspicious =
        preferences.getUChar(
            KEY_SCORE_SUSPICIOUS,
            settings.scoreSuspicious
        );

    settings.scoreProbable =
        preferences.getUChar(
            KEY_SCORE_PROBABLE,
            settings.scoreProbable
        );

    settings.scoreHighConfidence =
        preferences.getUChar(
            KEY_SCORE_HIGH_CONFIDENCE,
            settings.scoreHighConfidence
        );

    preferences.end();


    // =============================================================================================
    // VALIDAR CONFIGURACIÓN CARGADA
    // =============================================================================================

    /*
     * NVS podría contener valores antiguos, dañados o incompatibles.
     *
     * En ese caso se descarta completamente la configuración almacenada
     * y se utilizan los valores predeterminados del firmware.
     */
    if (!validateDetectionSettings(settings))
    {
        Serial0.println("[Settings] ADVERTENCIA: configuración almacenada no válida.");

        Serial0.println("[Settings] Se utilizarán los valores predeterminados.");

        return DetectionSettings{};
    }

    Serial0.println("[Settings] Configuración de detección cargada.");

    return settings;
}


// -------------------------------------------------------------------------------------------------
// GUARDAR CONFIGURACIÓN DE DETECCIÓN
// -------------------------------------------------------------------------------------------------

bool SettingsManager::saveDetectionSettings(const DetectionSettings& settings)
{
    // =============================================================================================
    // VALIDAR CONFIGURACIÓN
    // =============================================================================================

    if (!validateDetectionSettings(settings))
    {
        Serial0.println("[Settings] ERROR: la configuración de detección no es válida.");
        return false;
    }

    Preferences preferences;

    if (!preferences.begin(DETECTION_NAMESPACE, false))
    {
        Serial0.println("[Settings] ERROR: no fue posible abrir NVS para escritura.");
        return false;
    }

    bool success = true;

    // =============================================================================================
    // LÍNEA BASE
    // =============================================================================================

    success &=
        preferences.putULong(
            KEY_BASELINE_LEARNING_MS,
            settings.baselineLearningMs
        ) > 0;


    success &=
        preferences.putUShort(
            KEY_MIN_BASELINE_SAMPLES,
            settings.minBaselineSamples
        ) > 0;


    success &=
        preferences.putFloat(
            KEY_BASELINE_ALPHA,
            settings.baselineAlpha
        ) > 0;


    // =============================================================================================
    // UMBRALES
    // =============================================================================================

    success &=
        preferences.putFloat(
            KEY_PM25_DELTA,
            settings.pm25DeltaThreshold
        ) > 0;

    success &=
        preferences.putFloat(
            KEY_FINE_PARTICLE_RATIO,
            settings.fineParticleRatioThreshold
        ) > 0;

    success &=
        preferences.putFloat(
            KEY_VOC_DELTA,
            settings.vocDeltaThreshold
        ) > 0;

    success &=
        preferences.putFloat(
            KEY_NOX_DELTA,
            settings.noxDeltaThreshold
        ) > 0;

    success &=
        preferences.putUShort(
            KEY_PERSISTENCE_SAMPLES,
            settings.persistenceSamples
        ) > 0;

    // =============================================================================================
    // PUNTUACIONES
    // =============================================================================================

    success &=
        preferences.putUChar(
            KEY_SCORE_PARTICLES,
            settings.scoreParticles
        ) > 0;

    success &=
        preferences.putUChar(
            KEY_SCORE_FINE_PARTICLES,
            settings.scoreFineParticles
        ) > 0;

    success &=
        preferences.putUChar(
            KEY_SCORE_VOC,
            settings.scoreVoc
        ) > 0;

    success &=
        preferences.putUChar(
            KEY_SCORE_NOX,
            settings.scoreNox
        ) > 0;

    success &=
        preferences.putUChar(
            KEY_SCORE_PERSISTENCE,
            settings.scorePersistence
        ) > 0;

    // =============================================================================================
    // ESTADOS
    // =============================================================================================

    success &=
        preferences.putUChar(
            KEY_SCORE_SUSPICIOUS,
            settings.scoreSuspicious
        ) > 0;

    success &=
        preferences.putUChar(
            KEY_SCORE_PROBABLE,
            settings.scoreProbable
        ) > 0;

    success &=
        preferences.putUChar(
            KEY_SCORE_HIGH_CONFIDENCE,
            settings.scoreHighConfidence
        ) > 0;

    preferences.end();

    // =============================================================================================
    // COMPROBAR RESULTADO
    // =============================================================================================

    if (!success)
    {
        Serial0.println("[Settings] ERROR: uno o más parámetros no pudieron guardarse.");
        return false;
    }

    Serial0.println("[Settings] Configuración de detección guardada.");

    return true;
}

// -------------------------------------------------------------------------------------------------
// RESTAURAR CONFIGURACIÓN DE DETECCIÓN
// -------------------------------------------------------------------------------------------------

bool SettingsManager::resetDetectionSettings()
{
    Preferences preferences;

    if (!preferences.begin(DETECTION_NAMESPACE, false))
    {
        Serial0.println("[Settings] ERROR: no fue posible abrir la configuración de detección.");
        return false;
    }

    /*
     * Eliminar todas las claves del namespace.
     *
     * La próxima llamada a loadDetectionSettings() devolverá los
     * valores predeterminados de DetectionSettings.
     */
    const bool success =preferences.clear();

    preferences.end();

    if (!success)
    {
        Serial0.println("[Settings] ERROR: no fue posible restaurar la configuración.");
        return false;
    }

    Serial0.println("[Settings] Configuración de detección restaurada.");

    return true;
}

// -------------------------------------------------------------------------------------------------
// VALIDAR CONFIGURACIÓN DE DETECCIÓN
// -------------------------------------------------------------------------------------------------

bool SettingsManager::validateDetectionSettings(const DetectionSettings& settings) const
{
    // =============================================================================================
    // LÍNEA BASE
    // =============================================================================================

    if (settings.baselineLearningMs == 0)
    {
        return false;
    }


    if (settings.minBaselineSamples == 0)
    {
        return false;
    }


    if (
        !std::isfinite(settings.baselineAlpha) ||
        settings.baselineAlpha < 0.0f ||
        settings.baselineAlpha > 1.0f
    )
    {
        return false;
    }


    // =============================================================================================
    // UMBRALES
    // =============================================================================================

    if (!std::isfinite(settings.pm25DeltaThreshold) || settings.pm25DeltaThreshold < 0.0f)
    {
        return false;
    }


    if (
        !std::isfinite(settings.fineParticleRatioThreshold) ||
        settings.fineParticleRatioThreshold < 0.0f ||
        settings.fineParticleRatioThreshold > 1.0f
    )
    {
        return false;
    }

    if (
        !std::isfinite(settings.vocDeltaThreshold) ||
        settings.vocDeltaThreshold < 0.0f
    )
    {
        return false;
    }

    if (!std::isfinite(settings.noxDeltaThreshold) ||settings.noxDeltaThreshold < 0.0f)
    {
        return false;
    }

    if (settings.persistenceSamples == 0)
    {
        return false;
    }


    // =============================================================================================
    // PUNTUACIONES
    // =============================================================================================

    if (settings.scoreParticles > 100)
    {
        return false;
    }

    if (settings.scoreFineParticles > 100)
    {
        return false;
    }

    if (settings.scoreVoc > 100)
    {
        return false;
    }

    if (settings.scoreNox > 100)
    {
        return false;
    }

    if (settings.scorePersistence > 100)
    {
        return false;
    }

    // =============================================================================================
    // ESTADOS
    // =============================================================================================

    if (settings.scoreSuspicious > 100)
    {
        return false;
    }


    if (settings.scoreProbable > 100)
    {
        return false;
    }

    if (settings.scoreHighConfidence > 100)
    {
        return false;
    }

    /*
     * Los límites deben permanecer ordenados:
     *
     * Normal
     *   ↓
     * Suspicious
     *   ↓
     * SmokeProbable
     *   ↓
     * SmokeHighConfidence
     */
    if (
        settings.scoreSuspicious >= settings.scoreProbable ||
        settings.scoreProbable >= settings.scoreHighConfidence
    )
    {
        return false;
    }

    return true;
    
}

// -------------------------------------------------------------------------------------------------
// CARGAR REDES WI-FI
// -------------------------------------------------------------------------------------------------

uint8_t SettingsManager::loadWiFiNetworks(WiFiNetwork* networks, uint8_t capacity)
{
    if (networks == nullptr || capacity == 0)
    {
        return 0;
    }

    Preferences preferences;

    if (!preferences.begin(WIFI_NAMESPACE, true))
    {
        Serial0.println("[Settings] No existen redes Wi-Fi almacenadas.");
        return 0;
    }

    const uint8_t storedCount = preferences.getUChar(KEY_WIFI_COUNT, 0);

    const uint8_t maxCount =
        min(
            storedCount,
            static_cast<uint8_t>(
                min(
                    capacity,
                    Config::MAX_WIFI_NETWORKS
                )
            )
        );

    uint8_t loadedCount = 0;

    for (uint8_t index = 0; index < maxCount; index++)
    {
        WiFiNetwork network;


        // -----------------------------------------------------------------------------------------
        // LEER SSID
        // -----------------------------------------------------------------------------------------

        network.ssid = preferences.getString(makeWiFiSsidKey(index).c_str(), "");

        if (network.ssid.isEmpty())
        {
            Serial0.printf("[Settings] ADVERTENCIA: red Wi-Fi %u sin SSID.\n", index);
            continue;
        }


        // -----------------------------------------------------------------------------------------
        // LEER CONTRASEÑA CIFRADA
        // -----------------------------------------------------------------------------------------

        const String encryptedPassword =
            preferences.getString(makeWiFiPasswordKey(index).c_str(),"");


        // -----------------------------------------------------------------------------------------
        // DESCIFRAR CONTRASEÑA
        // -----------------------------------------------------------------------------------------

        if (!CryptoManager::decrypt(encryptedPassword, network.password))
        {
            /*
             * Si la contraseña no puede autenticarse o descifrarse,
             * la red completa se descarta.
             *
             * No debemos interpretarla como una red abierta porque eso
             * podría provocar intentos de conexión incorrectos.
             */
            Serial0.printf(
                "[Settings] ERROR: no fue posible descifrar la red '%s'.\n",
                network.ssid.c_str()
            );

            continue;
        }


        // -----------------------------------------------------------------------------------------
        // VALIDAR RED
        // -----------------------------------------------------------------------------------------

        if (!validateWiFiNetwork(network))
        {
            Serial0.printf(
                "[Settings] ADVERTENCIA: red Wi-Fi inválida: %s\n",
                network.ssid.c_str()
            );

            continue;
        }

        // -----------------------------------------------------------------------------------------
        // AGREGAR RED
        // -----------------------------------------------------------------------------------------

        networks[loadedCount] = network;

        loadedCount++;
    }


    preferences.end();

    Serial0.printf("[Settings] Redes Wi-Fi cargadas: %u\n", loadedCount);

    return loadedCount;
}

// -------------------------------------------------------------------------------------------------
// GUARDAR RED WI-FI
// -------------------------------------------------------------------------------------------------

bool SettingsManager::saveWiFiNetwork(const WiFiNetwork& network)
{
    // =============================================================================================
    // VALIDAR RED
    // =============================================================================================

    if (!validateWiFiNetwork(network))
    {
        Serial0.println("[Settings] ERROR: configuración Wi-Fi no válida.");
        return false;
    }

    // =============================================================================================
    // CARGAR REDES EXISTENTES
    // =============================================================================================

    WiFiNetwork networks[Config::MAX_WIFI_NETWORKS];

    uint8_t count = loadWiFiNetworks(networks, Config::MAX_WIFI_NETWORKS);

    // =============================================================================================
    // BUSCAR RED EXISTENTE
    // =============================================================================================

    for (uint8_t index = 0; index < count; index++)
    {
        if (networks[index].ssid == network.ssid)
        {
            /*
             * La red ya existe.
             *
             * Actualizamos sus credenciales sin modificar su posición.
             */
            networks[index] = network;

            if (!writeWiFiNetworks(networks, count))
            {
                return false;
            }

            Serial0.printf("[Settings] Red Wi-Fi actualizada: %s\n", network.ssid.c_str());

            return true;
        }
    }

    // =============================================================================================
    // COMPROBAR ESPACIO
    // =============================================================================================

    if (count >= Config::MAX_WIFI_NETWORKS)
    {
        Serial0.println("[Settings] ERROR: se alcanzó el máximo de redes Wi-Fi almacenadas.");
        return false;
    }

    // =============================================================================================
    // AGREGAR NUEVA RED
    // =============================================================================================

    networks[count] = network;

    count++;

    if (!writeWiFiNetworks(networks, count))
    {
        return false;
    }

    Serial0.printf("[Settings] Red Wi-Fi agregada: %s\n", network.ssid.c_str());

    return true;
}

// -------------------------------------------------------------------------------------------------
// ELIMINAR RED WI-FI
// -------------------------------------------------------------------------------------------------

bool SettingsManager::deleteWiFiNetwork(const String& ssid)
{
    if (ssid.isEmpty())
    {
        return false;
    }

    WiFiNetwork networks[Config::MAX_WIFI_NETWORKS];

    const uint8_t count = loadWiFiNetworks(networks, Config::MAX_WIFI_NETWORKS);

    WiFiNetwork remainingNetworks[Config::MAX_WIFI_NETWORKS];


    uint8_t remainingCount = 0;

    bool found = false;


    // =============================================================================================
    // COPIAR REDES QUE NO SERÁN ELIMINADAS
    // =============================================================================================

    for (uint8_t index = 0; index < count; index++)
    {
        if (networks[index].ssid == ssid)
        {
            found = true;
            continue;
        }

        remainingNetworks[remainingCount] = networks[index];

        remainingCount++;
    }


    // =============================================================================================
    // COMPROBAR EXISTENCIA
    // =============================================================================================

    if (!found)
    {
        Serial0.printf("[Settings] Red Wi-Fi no encontrada: %s\n", ssid.c_str());
        return false;
    }


    // =============================================================================================
    // GUARDAR LISTA ACTUALIZADA
    // =============================================================================================

    if (!writeWiFiNetworks(remainingNetworks, remainingCount))
    {
        return false;
    }

    Serial0.printf("[Settings] Red Wi-Fi eliminada: %s\n", ssid.c_str());


    return true;
}

// -------------------------------------------------------------------------------------------------
// ELIMINAR TODAS LAS REDES WI-FI
// -------------------------------------------------------------------------------------------------

bool SettingsManager::clearWiFiNetworks()
{
    Preferences preferences;


    if (!preferences.begin(WIFI_NAMESPACE, false))
    {
        Serial0.println("[Settings] ERROR: no fue posible acceder a las redes Wi-Fi.");
        return false;
    }

    const bool success = preferences.clear();

    preferences.end();


    if (!success)
    {
        Serial0.println("[Settings] ERROR: no fue posible eliminar las redes Wi-Fi.");
        return false;
    }

    Serial0.println("[Settings] Todas las redes Wi-Fi fueron eliminadas.");
    return true;
}

// -------------------------------------------------------------------------------------------------
// ESCRIBIR REDES WI-FI
// -------------------------------------------------------------------------------------------------

bool SettingsManager::writeWiFiNetworks(const WiFiNetwork* networks, uint8_t count)
{
    // =============================================================================================
    // VALIDAR PARÁMETROS
    // =============================================================================================

    if (count > Config::MAX_WIFI_NETWORKS)
    {
        return false;
    }


    if (count > 0 && networks == nullptr)
    {
        return false;
    }

    // =============================================================================================
    // CIFRAR CONTRASEÑAS
    // =============================================================================================

    /*
     * Ciframos todas las contraseñas antes de abrir NVS.
     *
     * De esta manera, si ocurre un error criptográfico, no comenzamos
     * a modificar parcialmente la configuración almacenada.
     */
    String encryptedPasswords[Config::MAX_WIFI_NETWORKS];


    for (uint8_t index = 0; index < count; index++)
    {
        if (!validateWiFiNetwork(networks[index]))
        {
            Serial0.printf(
                "[Settings] ERROR: red Wi-Fi inválida: %s\n",
                networks[index].ssid.c_str()
            );

            return false;
        }


        if (!CryptoManager::encrypt(networks[index].password, encryptedPasswords[index]))
        {
            Serial0.printf(
                "[Settings] ERROR: no fue posible cifrar la contraseña de '%s'.\n",
                networks[index].ssid.c_str()
            );

            return false;
        }
    }


    // =============================================================================================
    // ABRIR NVS
    // =============================================================================================

    Preferences preferences;

    if (!preferences.begin(WIFI_NAMESPACE, false))
    {
        Serial0.println("[Settings] ERROR: no fue posible abrir NVS para guardar redes Wi-Fi.");

        return false;
    }

    bool success = true;

    // =============================================================================================
    // GUARDAR REDES
    // =============================================================================================

    for (uint8_t index = 0; index < count; index++)
    {
        const String ssidKey = makeWiFiSsidKey(index);

        const String passwordKey = makeWiFiPasswordKey(index);

        // -----------------------------------------------------------------------------------------
        // GUARDAR SSID
        // -----------------------------------------------------------------------------------------

        success &= preferences.putString(ssidKey.c_str(), networks[index].ssid) > 0;

        // -----------------------------------------------------------------------------------------
        // GUARDAR CONTRASEÑA CIFRADA
        // -----------------------------------------------------------------------------------------

        success &= preferences.putString(passwordKey.c_str(), encryptedPasswords[index]) > 0;
    }

    // =============================================================================================
    // ELIMINAR POSICIONES NO UTILIZADAS
    // =============================================================================================

    /*
     * Si anteriormente existían más redes, eliminamos las posiciones
     * que dejaron de utilizarse.
     */
    for (uint8_t index = count; index < Config::MAX_WIFI_NETWORKS; index++)
    {
        const String ssidKey = makeWiFiSsidKey(index);

        const String passwordKey = makeWiFiPasswordKey(index);

        preferences.remove(ssidKey.c_str());

        preferences.remove(passwordKey.c_str());
    }


    // =============================================================================================
    // GUARDAR CANTIDAD
    // =============================================================================================

    success &= preferences.putUChar(KEY_WIFI_COUNT, count) > 0;

    // =============================================================================================
    // VERIFICAR CANTIDAD
    // =============================================================================================

    success &= preferences.getUChar(KEY_WIFI_COUNT, 255) == count;

    preferences.end();

    // =============================================================================================
    // COMPROBAR RESULTADO
    // =============================================================================================

    if (!success)
    {
        Serial0.println(
            "[Settings] ERROR: no fue posible guardar completamente la lista de redes Wi-Fi."
        );

        return false;
    }

    return true;
}

// -------------------------------------------------------------------------------------------------
// VALIDAR RED WI-FI
// -------------------------------------------------------------------------------------------------

bool SettingsManager::validateWiFiNetwork(const WiFiNetwork& network) const
{
    // El SSID no puede estar vacío.
    if (network.ssid.isEmpty())
    {
        return false;
    }

    /*
     * Un SSID IEEE 802.11 puede ocupar hasta 32 bytes.
     */
    if (network.ssid.length() > 32)
    {
        return false;
    }

    /*
     * Para WPA/WPA2 una contraseña convencional puede contener
     * hasta 63 caracteres.
     *
     * Se permite una contraseña vacía porque la red podría ser abierta.
     */
    if (network.password.length() > 63)
    {
        return false;
    }

    return true;
}