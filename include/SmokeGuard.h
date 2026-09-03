#pragma once
// =================================================================================================
#include <Arduino.h>
// =================================================================================================
#include "SensorData.h"
#include "DetectionSettings.h"
// =================================================================================================


/**
 * @enum SmokeState
 * @brief Estados posibles del análisis de humo realizado por SmokeGuard.
 */
enum class SmokeState : uint8_t
{
    InsufficientData,
    WarmingUp,
    Normal,
    Suspicious,
    SmokeProbable,
    SmokeHighConfidence
};


/**
 * @struct SmokeAnalysis
 * @brief Resultado del análisis realizado por SmokeGuard.
 *
 * Contiene el estado determinado, la puntuación de probabilidad de humo
 * y las principales evidencias utilizadas durante el análisis.
 */
struct SmokeAnalysis
{
    /**
     * @brief Estado determinado por el análisis.
     */
    SmokeState state = SmokeState::InsufficientData;


    /**
     * @brief Puntuación estimada de presencia de humo.
     *
     * El valor se encuentra entre 0 y 100.
     */
    uint8_t score = 0;


    /**
     * @brief Relación entre PM1.0 y PM2.5.
     *
     * Permite estimar qué proporción del material particulado fino
     * corresponde a partículas de hasta 1 µm.
     */
    float fineParticleRatio = 0.0f;


    /**
     * @brief Variación de PM2.5 respecto de la línea base.
     */
    float pm25Delta = 0.0f;


    /**
     * @brief Variación del índice VOC respecto de la línea base.
     */
    float vocDelta = 0.0f;


    /**
     * @brief Variación del índice NOx respecto de la línea base.
     */
    float noxDelta = 0.0f;


    /**
     * @brief Variación de humedad respecto de la línea base.
     */
    float humidityDelta = 0.0f;


    /**
     * @brief Indica si existe evidencia relevante de material particulado.
     */
    bool particleEvidence = false;


    /**
     * @brief Indica si existe evidencia relevante de partículas finas.
     */
    bool fineParticleEvidence = false;


    /**
     * @brief Indica si existe evidencia relevante de VOC.
     */
    bool vocEvidence = false;


    /**
     * @brief Indica si existe evidencia relevante de NOx.
     */
    bool noxEvidence = false;


    /**
     * @brief Indica si el evento sospechoso ha persistido durante varias muestras.
     */
    bool persistenceEvidence = false;
};


/**
 * @class SmokeGuard
 * @file SmokeGuard.h
 * @brief Analiza los datos ambientales para detectar eventos compatibles con humo.
 *
 * SmokeGuard combina las mediciones de material particulado, VOC, NOx,
 * temperatura y humedad para estimar si las condiciones observadas
 * corresponden a un posible evento de humo.
 *
 * La detección utiliza cambios respecto de una línea base ambiental,
 * distribución del tamaño de las partículas y persistencia temporal.
 */
class SmokeGuard
{
    public:

     /**
         * @brief Establece la configuración utilizada por el algoritmo de detección de SmokeGuard.
         * 
         * @param settings Configuración para el análisis de humo.
         */
        void setSettings(const DetectionSettings& settings);

        /**
         * @brief Inicializa el sistema de análisis de humo.
         *
         * Reinicia las líneas base, contadores y estados internos.
         * 
         * @param settings Configuración para el análisis de humo.
         */
        void begin(const DetectionSettings& settings);


        /**
         * @brief Analiza una nueva muestra de los sensores.
         *
         * Compara los datos actuales con las líneas base ambientales
         * y calcula una puntuación de probabilidad de humo.
         *
         * @param data Datos actuales obtenidos desde los sensores.
         *
         * @return Resultado completo del análisis.
         */
        SmokeAnalysis analyze(const SensorData& data);


        /**
         * @brief Reinicia completamente el sistema de análisis.
         *
         * Elimina las líneas base aprendidas y devuelve SmokeGuard
         * al período inicial de aprendizaje.
         */
        void reset();


    private:

        // =========================================================================================
        // PARÁMETROS DE DETECCIÓN
        // =========================================================================================

        /**
         * @brief Configuración utilizada por el algoritmo de detección de SmokeGuard.
         */
        DetectionSettings settings;

        // =========================================================================================
        // ESTADO
        // =========================================================================================

        /**
         * @brief Estado actual determinado por SmokeGuard.
         */
        SmokeState currentState = SmokeState::WarmingUp;


        /**
         * @brief Indica si SmokeGuard fue inicializado.
         */
        bool initialized = false;


        /**
         * @brief Momento en que comenzó el período de aprendizaje.
         */
        uint32_t startTime = 0;


        /**
         * @brief Momento en que fue procesada la última muestra.
         */
        uint32_t lastSampleTime = 0;


        /**
         * @brief Cantidad de muestras utilizadas para construir la línea base.
         */
        uint16_t baselineSamples = 0;


        /**
         * @brief Cantidad consecutiva de muestras consideradas sospechosas.
         */
        uint16_t suspiciousSamples = 0;

        /**
         * @brief Último análisis realizado.
         */
        SmokeAnalysis lastAnalysis;

        /**
         * @brief Cantidad de muestras válidas utilizadas para construir la línea base de NOx.
         */
        uint16_t baselineNoxSamples = 0;


        // =========================================================================================
        // LÍNEAS BASE
        // =========================================================================================

        /**
         * @brief Valor base de PM1.0.
         */
        float baselinePm1_0 = 0.0f;


        /**
         * @brief Valor base de PM2.5.
         */
        float baselinePm2_5 = 0.0f;


        /**
         * @brief Valor base de PM10.
         */
        float baselinePm10 = 0.0f;


        /**
         * @brief Valor base del índice VOC.
         */
        float baselineVocIndex = 0.0f;


        /**
         * @brief Valor base del índice NOx.
         */
        float baselineNoxIndex = 0.0f;


        /**
         * @brief Valor base de temperatura.
         */
        float baselineTemperature = 0.0f;


        /**
         * @brief Valor base de humedad relativa.
         */
        float baselineHumidity = 0.0f;


        // =========================================================================================
        // MÉTODOS INTERNOS
        // =========================================================================================

        /**
         * @brief Comprueba si existen datos suficientes para realizar el análisis.
         *
         * @param data Datos actuales de los sensores.
         *
         * @retval true Si existen datos suficientes.
         * @retval false Si faltan mediciones esenciales.
         */
        bool hasMinimumData(const SensorData& data) const;


        /**
         * @brief Comprueba si SmokeGuard continúa en el período inicial de aprendizaje.
         *
         * @retval true Si todavía se encuentra aprendiendo las condiciones ambientales.
         * @retval false Si la línea base inicial está disponible.
         */
        bool isWarmingUp() const;


        /**
         * @brief Actualiza las líneas base ambientales.
         *
         * @param data Datos actuales de los sensores.
         */
        void updateBaseline(const SensorData& data);


        /**
         * @brief Calcula la relación entre PM1.0 y PM2.5.
         *
         * @param data Datos actuales de los sensores.
         *
         * @return Relación PM1.0 / PM2.5.
         */
        float calculateFineParticleRatio(const SensorData& data) const;


        /**
         * @brief Calcula la puntuación de probabilidad de humo.
         *
         * También actualiza las evidencias encontradas durante el análisis.
         *
         * @param data Datos actuales de los sensores.
         * @param analysis Resultado donde se almacenan las evidencias detectadas.
         *
         * @return Puntuación entre 0 y 100.
         */
        uint8_t calculateScore(
            const SensorData& data,
            SmokeAnalysis& analysis
        ) const;


        /**
         * @brief Actualiza el contador de persistencia de eventos sospechosos.
         *
         * @param suspicious Indica si la muestra actual presenta condiciones sospechosas.
         */
        void updatePersistence(bool suspicious);


        /**
         * @brief Determina el estado de SmokeGuard según la puntuación calculada.
         *
         * @param score Puntuación de probabilidad de humo.
         *
         * @return Estado correspondiente a la puntuación.
         */
        SmokeState determineState(uint8_t score) const;
};