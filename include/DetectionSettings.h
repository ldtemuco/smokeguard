#pragma once
// =================================================================================================
#include <Arduino.h>
// =================================================================================================
#include "Config.h"
// =================================================================================================

/**
 * @struct DetectionSettings
 * @brief Configuración utilizada por el algoritmo de detección de SmokeGuard.
 *
 * Los valores iniciales corresponden a los valores predeterminados
 * definidos en Config.h.
 */
struct DetectionSettings
{
    // =============================================================================================
    // LÍNEA BASE
    // =============================================================================================

    uint32_t baselineLearningMs = Config::SMOKEGUARD_BASELINE_LEARNING_MS;

    uint16_t minBaselineSamples = Config::SMOKEGUARD_MIN_BASELINE_SAMPLES;

    float baselineAlpha = Config::SMOKEGUARD_BASELINE_ALPHA;

    // =============================================================================================
    // UMBRALES DE EVIDENCIA
    // =============================================================================================

    float pm25DeltaThreshold = Config::SMOKEGUARD_PM25_DELTA_THRESHOLD;

    float fineParticleRatioThreshold = Config::SMOKEGUARD_FINE_PARTICLE_RATIO_THRESHOLD;

    float vocDeltaThreshold = Config::SMOKEGUARD_VOC_DELTA_THRESHOLD;

    float noxDeltaThreshold = Config::SMOKEGUARD_NOX_DELTA_THRESHOLD;

    uint16_t persistenceSamples = Config::SMOKEGUARD_PERSISTENCE_SAMPLES;

    // =============================================================================================
    // PUNTUACIONES
    // =============================================================================================

    uint8_t scoreParticles = Config::SMOKEGUARD_SCORE_PARTICLES;

    uint8_t scoreFineParticles = Config::SMOKEGUARD_SCORE_FINE_PARTICLES;

    uint8_t scoreVoc = Config::SMOKEGUARD_SCORE_VOC;

    uint8_t scoreNox = Config::SMOKEGUARD_SCORE_NOX;

    uint8_t scorePersistence = Config::SMOKEGUARD_SCORE_PERSISTENCE;

    // =============================================================================================
    // ESTADOS
    // =============================================================================================

    uint8_t scoreSuspicious = Config::SMOKEGUARD_SCORE_SUSPICIOUS;

    uint8_t scoreProbable = Config::SMOKEGUARD_SCORE_PROBABLE;

    uint8_t scoreHighConfidence = Config::SMOKEGUARD_SCORE_HIGH_CONFIDENCE;
};