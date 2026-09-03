#pragma once
// =================================================================================================
#include <Arduino.h>
// =================================================================================================
/**
 * @class CryptoManager
 * @file CryptoManager.h
 * @brief Proporciona funciones criptográficas utilizadas por SmokeGuard.
 *
 * Permite cifrar y descifrar información sensible utilizando AES-256-GCM.
 *
 * Los datos cifrados se representan como una cadena Base64 para facilitar
 * su almacenamiento mediante Preferences en la memoria NVS del ESP32-S3.
 *
 * La clave utilizada por AES-256 se encuentra definida en Config.h.
 */
class CryptoManager
{
    public:

        /**
         * @brief Cifra una cadena utilizando AES-256-GCM.
         *
         * Para cada operación se genera un nonce aleatorio independiente.
         *
         * El resultado contiene la información necesaria para recuperar
         * posteriormente el texto original:
         *
         * nonce + texto cifrado + tag de autenticación.
         *
         * El conjunto completo se codifica en Base64 antes de ser devuelto.
         *
         * @param plainText Texto que se desea cifrar.
         * @param encryptedText Cadena donde se almacenará el resultado cifrado
         * codificado en Base64.
         *
         * @retval true Si el cifrado fue realizado correctamente.
         * @retval false Si ocurrió un error durante el proceso.
         */
        static bool encrypt(const String& plainText, String& encryptedText);


        /**
         * @brief Descifra una cadena previamente cifrada mediante AES-256-GCM.
         *
         * Decodifica la representación Base64, recupera el nonce,
         * el texto cifrado y el tag de autenticación, y verifica
         * la integridad de los datos antes de devolver el texto original.
         *
         * @param encryptedText Texto cifrado codificado en Base64.
         * @param plainText Cadena donde se almacenará el texto descifrado.
         *
         * @retval true Si los datos fueron autenticados y descifrados correctamente.
         * @retval false Si los datos son inválidos, fueron modificados o ocurrió un error.
         */
        static bool decrypt(const String& encryptedText, String& plainText);
};