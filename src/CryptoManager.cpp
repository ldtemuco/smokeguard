#include "CryptoManager.h"
#include "Config.h"
// =================================================================================================
#include <esp_random.h>
#include <mbedtls/base64.h>
#include <mbedtls/gcm.h>
#include <vector>
// =================================================================================================


namespace
{
    // =============================================================================================
    // PARÁMETROS AES-GCM
    // =============================================================================================

    /**
     * @brief Tamaño del nonce utilizado por AES-GCM.
     *
     * 12 bytes (96 bits) es el tamaño estándar recomendado para GCM.
     */
    constexpr size_t NONCE_SIZE = 12;


    /**
     * @brief Tamaño del tag de autenticación generado por AES-GCM.
     */
    constexpr size_t TAG_SIZE = 16;


    /**
     * @brief Tamaño de la clave AES expresado en bits.
     */
    constexpr unsigned int AES_KEY_BITS = 256;
}


// -------------------------------------------------------------------------------------------------
// CIFRAR TEXTO
// -------------------------------------------------------------------------------------------------

bool CryptoManager::encrypt(const String& plainText, String& encryptedText)
{
    // Limpiar el resultado antes de comenzar.
    encryptedText = "";


    // ---------------------------------------------------------------------------------------------
    // CADENA VACÍA
    // ---------------------------------------------------------------------------------------------

    /*
     * Una contraseña vacía representa una red Wi-Fi abierta.
     *
     * No es necesario generar un bloque AES-GCM para este caso.
     */
    if (plainText.isEmpty())
    {
        return true;
    }


    // ---------------------------------------------------------------------------------------------
    // PREPARAR DATOS
    // ---------------------------------------------------------------------------------------------

    const size_t plainTextLength = plainText.length();

    uint8_t nonce[NONCE_SIZE]{};

    uint8_t tag[TAG_SIZE]{};


    /*
     * Generar un nonce diferente para cada operación de cifrado.
     *
     * La reutilización del mismo nonce con la misma clave AES-GCM
     * comprometería la seguridad del cifrado.
     */
    esp_fill_random(
        nonce,
        NONCE_SIZE
    );


    // Buffer donde se almacenará exclusivamente el texto cifrado.
    std::vector<uint8_t> cipherText(plainTextLength);


    // ---------------------------------------------------------------------------------------------
    // INICIALIZAR AES-GCM
    // ---------------------------------------------------------------------------------------------

    mbedtls_gcm_context context;

    mbedtls_gcm_init(&context);

    // ---------------------------------------------------------------------------------------------
    // CONFIGURAR CLAVE AES-256
    // ---------------------------------------------------------------------------------------------

    const int keyResult =
        mbedtls_gcm_setkey(
            &context,
            MBEDTLS_CIPHER_ID_AES,
            Config::WIFI_AES_KEY,
            AES_KEY_BITS
        );


    if (keyResult != 0)
    {
        Serial0.printf("[Crypto] ERROR: no fue posible configurar AES. Código: %d\n", keyResult);

        mbedtls_gcm_free(&context);

        return false;
    }


    // ---------------------------------------------------------------------------------------------
    // CIFRAR
    // ---------------------------------------------------------------------------------------------

    /*
     * AES-GCM produce:
     *
     * - texto cifrado
     * - tag de autenticación
     *
     * No utilizamos Additional Authenticated Data (AAD), por lo que
     * los parámetros correspondientes son nullptr y 0.
     */
    const int encryptResult =
        mbedtls_gcm_crypt_and_tag(
            &context,
            MBEDTLS_GCM_ENCRYPT,
            plainTextLength,
            nonce,
            NONCE_SIZE,
            nullptr,
            0,
            reinterpret_cast<const uint8_t*>(
                plainText.c_str()
            ),
            cipherText.data(),
            TAG_SIZE,
            tag
        );


    mbedtls_gcm_free(&context);

    if (encryptResult != 0)
    {
        Serial0.printf("[Crypto] ERROR: fallo durante el cifrado. Código: %d\n", encryptResult);

        return false;
    }


    // ---------------------------------------------------------------------------------------------
    // CONSTRUIR BLOQUE CIFRADO
    // ---------------------------------------------------------------------------------------------

    /*
     * Formato interno:
     *
     * ┌──────────────┬───────────────────────┬────────────────┐
     * │ Nonce        │ Ciphertext            │ Tag            │
     * │ 12 bytes     │ tamaño variable       │ 16 bytes       │
     * └──────────────┴───────────────────────┴────────────────┘
     */

    const size_t binaryLength =
        NONCE_SIZE +
        plainTextLength +
        TAG_SIZE;


    std::vector<uint8_t> binaryData(
        binaryLength
    );


    size_t offset = 0;


    // Copiar nonce.
    memcpy(
        binaryData.data() + offset,
        nonce,
        NONCE_SIZE
    );


    offset += NONCE_SIZE;


    // Copiar texto cifrado.
    memcpy(
        binaryData.data() + offset,
        cipherText.data(),
        plainTextLength
    );


    offset += plainTextLength;


    // Copiar tag.
    memcpy(
        binaryData.data() + offset,
        tag,
        TAG_SIZE
    );


    // ---------------------------------------------------------------------------------------------
    // CALCULAR TAMAÑO BASE64
    // ---------------------------------------------------------------------------------------------

    /*
     * Base64 utiliza cuatro caracteres por cada grupo de tres bytes.
     *
     * Se reserva un byte adicional para el terminador nulo.
     */
    const size_t base64Capacity = 4 * ((binaryLength + 2) / 3) + 1;

    std::vector<uint8_t> base64Data(base64Capacity);

    size_t base64Length = 0;


    // ---------------------------------------------------------------------------------------------
    // CODIFICAR BASE64
    // ---------------------------------------------------------------------------------------------

    const int base64Result =
        mbedtls_base64_encode(
            base64Data.data(),
            base64Data.size(),
            &base64Length,
            binaryData.data(),
            binaryData.size()
        );


    if (base64Result != 0)
    {
        Serial0.printf(
            "[Crypto] ERROR: no fue posible codificar Base64. Código: %d\n",
            base64Result
        );


        return false;
    }

    /*
     * Añadir manualmente terminador nulo.
     *
     * mbedtls_base64_encode() informa la longitud de los datos
     * generados sin contar dicho terminador.
     */
    base64Data[base64Length] = '\0';

    encryptedText = String(reinterpret_cast<const char*>(base64Data.data()));


    return true;
}


// -------------------------------------------------------------------------------------------------
// DESCIFRAR TEXTO
// -------------------------------------------------------------------------------------------------

bool CryptoManager::decrypt(const String& encryptedText, String& plainText)
{
    // Limpiar resultado.
    plainText = "";

    // ---------------------------------------------------------------------------------------------
    // CADENA VACÍA
    // ---------------------------------------------------------------------------------------------

    /*
     * Una cadena cifrada vacía representa una contraseña vacía,
     * utilizada normalmente por una red Wi-Fi abierta.
     */
    if (encryptedText.isEmpty())
    {
        return true;
    }


    // ---------------------------------------------------------------------------------------------
    // PREPARAR BUFFER BASE64
    // ---------------------------------------------------------------------------------------------

    const size_t encryptedLength = encryptedText.length();

    /*
     * El tamaño máximo del resultado de decodificar Base64 es
     * aproximadamente tres cuartos del tamaño original.
     *
     * Se añaden algunos bytes adicionales para simplificar el cálculo.
     */
    const size_t decodedCapacity = ((encryptedLength + 3) / 4) * 3;

    std::vector<uint8_t> binaryData(decodedCapacity);

    size_t binaryLength = 0;


    // ---------------------------------------------------------------------------------------------
    // DECODIFICAR BASE64
    // ---------------------------------------------------------------------------------------------

    const int base64Result =
        mbedtls_base64_decode(
            binaryData.data(),
            binaryData.size(),
            &binaryLength,
            reinterpret_cast<const uint8_t*>(
                encryptedText.c_str()
            ),
            encryptedLength
        );


    if (base64Result != 0)
    {
        Serial0.printf("[Crypto] ERROR: Base64 inválido. Código: %d\n", base64Result);

        return false;
    }


    // ---------------------------------------------------------------------------------------------
    // VALIDAR TAMAÑO
    // ---------------------------------------------------------------------------------------------

    /*
     * Un bloque válido necesita al menos:
     *
     * nonce + tag
     *
     * El ciphertext puede tener longitud cero, aunque en nuestro caso
     * las cadenas vacías se manejan anteriormente.
     */
    if (binaryLength < NONCE_SIZE + TAG_SIZE)
    {
        Serial0.println("[Crypto] ERROR: datos cifrados incompletos.");

        return false;
    }


    // ---------------------------------------------------------------------------------------------
    // EXTRAER COMPONENTES
    // ---------------------------------------------------------------------------------------------

    const uint8_t* nonce =
        binaryData.data();


    const size_t cipherTextLength =
        binaryLength -
        NONCE_SIZE -
        TAG_SIZE;


    const uint8_t* cipherText =
        binaryData.data() +
        NONCE_SIZE;


    const uint8_t* tag =
        binaryData.data() +
        NONCE_SIZE +
        cipherTextLength;


    // Buffer para el texto descifrado.
    std::vector<uint8_t> decryptedData(cipherTextLength + 1);


    // ---------------------------------------------------------------------------------------------
    // INICIALIZAR AES-GCM
    // ---------------------------------------------------------------------------------------------

    mbedtls_gcm_context context;

    mbedtls_gcm_init(&context);


    // ---------------------------------------------------------------------------------------------
    // CONFIGURAR CLAVE AES-256
    // ---------------------------------------------------------------------------------------------

    const int keyResult =
        mbedtls_gcm_setkey(
            &context,
            MBEDTLS_CIPHER_ID_AES,
            Config::WIFI_AES_KEY,
            AES_KEY_BITS
        );


    if (keyResult != 0)
    {
        Serial0.printf(
            "[Crypto] ERROR: no fue posible configurar AES. Código: %d\n",
            keyResult
        );


        mbedtls_gcm_free(
            &context
        );


        return false;
    }


    // ---------------------------------------------------------------------------------------------
    // AUTENTICAR Y DESCIFRAR
    // ---------------------------------------------------------------------------------------------

    /*
     * mbedtls_gcm_auth_decrypt() verifica primero el tag GCM.
     *
     * Si los datos fueron modificados, el nonce es incorrecto,
     * el tag no coincide o se utiliza otra clave AES, la operación
     * devuelve un error.
     */
    const int decryptResult =
        mbedtls_gcm_auth_decrypt(
            &context,
            cipherTextLength,
            nonce,
            NONCE_SIZE,
            nullptr,
            0,
            tag,
            TAG_SIZE,
            cipherText,
            decryptedData.data()
        );


    mbedtls_gcm_free(
        &context
    );


    if (decryptResult != 0)
    {
        Serial0.printf(
            "[Crypto] ERROR: autenticación o descifrado fallido. Código: %d\n",
            decryptResult
        );


        return false;
    }


    // ---------------------------------------------------------------------------------------------
    // CONVERTIR A STRING
    // ---------------------------------------------------------------------------------------------

    decryptedData[cipherTextLength] = '\0';

    plainText = String( reinterpret_cast<const char*>(decryptedData.data()));

    return true;
}