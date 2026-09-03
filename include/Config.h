#pragma once

#include <Arduino.h>

/*
* GLOSARIO
* --------------------------------------------------------------------------------------------------
* STA = Station connection mode  | Modo de conexión de Estación 
*       El dispositivo se conecta a una red Wi-Fi existente creada por un router o punto de acceso.
*       Funciona como un cliente más.
*
* I2C = Inter-Integrated Circuit | Circuito Inter-Integrado
*       Protocolo de comunicación en serie que conecta circuitos integrados usando solo dos cables. 
*
* SDA = I2C Serial Data Line | Línea de Datos Serial I2C
*       Linea que transfiere los datos entre los dispositivos conectados.
*
* SCL = I2C Serial Clock Line | Línea de Reloj Serial I2C
*       Linea que sincroniza la transferencia de datos entre los dispositivos conectados.
*/

/**
 * @namespace Config
 * @brief Espacio de nombres para almacenar las configuraciones del dispositivo. 
**/
namespace Config 
{
    // Nombre del punto de acceso Wi-Fi del dispositivo.
    static constexpr char AP_SSID[] = "SmokeGuard-Setup";

    // Contraseña del punto de acceso Wi-Fi del dispositivo.
    static constexpr char AP_PASSWORD[] = "Setup1234!";

    // Nombre por defecto para el usuario administrador del dispositivo.
    static constexpr char DEFAULT_ADMIN_USER[] = "admin";

    // Contraseña por defecto para el usuario administrador del dispositivo.
    static constexpr char DEFAULT_ADMIN_PASSWORD[] = "ChangeMe123!";

    // Puerto utilizado por el servidor HTTP.
    static constexpr uint16_t HTTP_PORT = 80;

    // Puerto utilizado por el servidor DNS.
    static constexpr uint16_t DNS_PORT = 53;

    // Tiempo máximo de espera (en milisegundos) para la conexión a una red Wi-Fi en modo STA.
    static constexpr uint32_t STA_CONNECT_TIMEOUT_MS = 12000;

    // Tiempo máximo de espera (en milisegundos) para la conexión a un cliente en modo AP.
    static constexpr uint32_t SESSION_TIMEOUT_MS = 30UL * 60UL * 1000UL;

    // En SmokeGuard, el bus principal (I2C-0) se utiliza para los sensores SHT45 y SGP41, 
    // mientras que el bus secundario (I2C-1) se utiliza para el sensor SPS30.

    // Número de PIN GPIO para SDA en el bus principal (I2C-0). Lo usan los sensores SHT45 y SGP41.
    constexpr uint8_t SENSOR_SDA = 8;

    // Número de PIN GPIO para SCL en el bus principal (I2C-0). Lo usan los sensores SHT45 y SGP41.
    constexpr uint8_t SENSOR_SCL = 9;

    // Número de PIN GPIO para SDA en el bus secundario (I2C-1). Lo usa el sensor SPS30.
    constexpr uint8_t SPS30_SDA = 17;

    // Número de PIN GPIO para SCL en el bus secundario (I2C-1). Lo usa el sensor SPS30.
    constexpr uint8_t SPS30_SCL = 18;
   
    // Dirección I2C del sensor SHT45.
    constexpr uint8_t SHT45_ADDRESS = 0x44; // DEC = 68

    // Dirección I2C del sensor SGP41.
    constexpr uint8_t SGP41_ADDRESS = 0x59; // DEC = 89

    // Dirección I2C del sensor SPS30.
    constexpr uint8_t SPS30_ADDRESS = 0x69; // DEC = 105

    // Frecuencia de comunicación I2C en Hertz (Hz). La frecuencia estándar es de 100 kHz.
    constexpr uint32_t I2C_FREQUENCY = 100000;

    // Tiempo máximo de espera (en milisegundos) para la comunicación I2C.
    constexpr uint32_t I2C_TIMEOUT_MS = 100;

    // Intervalos de tiempo para la lectura de sensores y la impresión de datos en milisegundos.
    constexpr uint32_t SENSOR_INTERVAL_MS = 1000;

    // Intervalo de tiempo para la impresión de datos en milisegundos.
    constexpr uint32_t PRINT_INTERVAL_MS = 5000;

    // Tiempo de acondicionamiento del sensor SGP41 en segundos.
    constexpr uint8_t SGP41_CONDITIONING_SECONDS = 10;

    // Dominio utilizado por la API de SmokeGuard.
    constexpr char API_HOST[] = "smokeguard-api.bfarfal.workers.dev";

    // URL utilizada para enviar alertas.
    constexpr char API_ALERTS_URL[] = "https://smokeguard-api.bfarfal.workers.dev/api/v1/alerts";

    // Tiempo máximo de espera para establecer la conexión HTTPS en milisegundos.
    constexpr uint32_t API_CONNECT_TIMEOUT_MS = 5000;

    // Tiempo máximo de espera para recibir una respuesta HTTP en milisegundos.
    constexpr uint32_t API_RESPONSE_TIMEOUT_MS = 5000;

    // Tiempo máximo de espera para completar el handshake TLS en segundos.
    constexpr uint32_t API_TLS_HANDSHAKE_TIMEOUT_S = 5;

    // Servidores utilizados para sincronizar la hora del dispositivo mediante NTP.
    constexpr char NTP_SERVER_1[] = "pool.ntp.org";
    constexpr char NTP_SERVER_2[] = "time.nist.gov";
    constexpr char NTP_SERVER_3[] = "time.google.com";

    // Tiempo máximo de espera para sincronizar el reloj mediante NTP.
    constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 10000;

    // Año mínimo considerado válido para comprobar que el reloj fue sincronizado.
    constexpr uint16_t NTP_MIN_VALID_YEAR = 2024;

    // =============================================================================================
    // SMOKEGUARD
    // =============================================================================================

    // Intervalo mínimo entre análisis consecutivos.
    // Este valor forma parte del funcionamiento interno y no es configurable desde el panel.
    constexpr uint32_t SMOKEGUARD_SAMPLE_INTERVAL_MS = 1000;


    // =============================================================================================
    // VALORES PREDETERMINADOS DE DETECCIÓN
    // =============================================================================================

    // Tiempo predeterminado utilizado para aprender las condiciones ambientales normales.
    constexpr uint32_t SMOKEGUARD_BASELINE_LEARNING_MS = 60000;

    // Cantidad mínima predeterminada de muestras necesarias para construir la línea base inicial.
    constexpr uint16_t SMOKEGUARD_MIN_BASELINE_SAMPLES = 30;

    // Factor predeterminado utilizado para adaptar lentamente la línea base.
    constexpr float SMOKEGUARD_BASELINE_ALPHA = 0.02f;


    // =============================================================================================
    // UMBRALES DE EVIDENCIA PREDETERMINADOS
    // =============================================================================================

    // Incremento mínimo predeterminado de PM2.5 respecto de la línea base.
    constexpr float SMOKEGUARD_PM25_DELTA_THRESHOLD = 10.0f;

    // Relación mínima predeterminada PM1.0 / PM2.5 considerada indicativa de partículas finas.
    constexpr float SMOKEGUARD_FINE_PARTICLE_RATIO_THRESHOLD = 0.70f;

    // Incremento mínimo predeterminado del índice VOC respecto de la línea base.
    constexpr float SMOKEGUARD_VOC_DELTA_THRESHOLD = 30.0f;

    // Incremento mínimo predeterminado del índice NOx respecto de la línea base.
    constexpr float SMOKEGUARD_NOX_DELTA_THRESHOLD = 10.0f;

    // Cantidad predeterminada de muestras sospechosas consecutivas necesarias para persistencia.
    constexpr uint16_t SMOKEGUARD_PERSISTENCE_SAMPLES = 5;


    // =============================================================================================
    // PUNTUACIONES PREDETERMINADAS
    // =============================================================================================

    constexpr uint8_t SMOKEGUARD_SCORE_PARTICLES = 30;

    constexpr uint8_t SMOKEGUARD_SCORE_FINE_PARTICLES = 15;

    constexpr uint8_t SMOKEGUARD_SCORE_VOC = 20;

    constexpr uint8_t SMOKEGUARD_SCORE_NOX = 15;

    constexpr uint8_t SMOKEGUARD_SCORE_PERSISTENCE = 20;

    // =============================================================================================
    // ESTADOS PREDETERMINADOS
    // =============================================================================================

    constexpr uint8_t SMOKEGUARD_SCORE_SUSPICIOUS = 30;

    constexpr uint8_t SMOKEGUARD_SCORE_PROBABLE = 60;

    constexpr uint8_t SMOKEGUARD_SCORE_HIGH_CONFIDENCE = 80;

    // =============================================================================================
    // RED
    // =============================================================================================

    // Cantidad máxima de redes Wi-Fi que pueden almacenarse.
    constexpr uint8_t MAX_WIFI_NETWORKS = 10;

    // =============================================================================================
    // SEGURIDAD
    // =============================================================================================

    /*
        En un entorno de producción, la clave AES-256 debería almacenarse en un lugar seguro, 
        como un módulo de seguridad de hardware (HSM) o un sistema de gestión de datos sensibles.     
        
        Si alguien tiene acceso al código fuente del firmware, puede descifrar todas las 
        credenciales Wi-Fi almacenadas en la memoria no volátil del dispositivo.
    */
    // Clave AES-256 utilizada para cifrar las credenciales Wi-Fi.
    
    constexpr uint8_t WIFI_AES_KEY[32] =
    {
        0x71, 0x3A, 0xC5, 0x29,
        0x8F, 0x14, 0x62, 0xD7,
        0x33, 0xE1, 0x95, 0x4B,
        0xA8, 0x20, 0xFC, 0x56,
        0x19, 0xB4, 0x73, 0xDE,
        0x42, 0x87, 0x0D, 0xCA,
        0x65, 0x31, 0xF9, 0x18,
        0xAC, 0x52, 0x7E, 0x93
    };

    // Cantidad de iteraciones PBKDF2 utilizadas para proteger la contraseña del panel.
    constexpr uint32_t ADMIN_PBKDF2_ITERATIONS = 20000;
}
