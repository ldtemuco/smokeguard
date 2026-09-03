#include "SmokeGuard.h"
#include "Config.h"
// =================================================================================================

// -------------------------------------------------------------------------------------------------
// INICIAR SMOKEGUARD
// -------------------------------------------------------------------------------------------------

void SmokeGuard::begin(const DetectionSettings& settings)
{
    // Guardar la configuración que utilizará el algoritmo de detección.
    this->settings = settings;

    initialized = true;

    reset();

    Serial0.println();
    Serial0.println("========================================");
    Serial0.println("           SMOKEGUARD INICIADO");
    Serial0.println("========================================");
}


// -------------------------------------------------------------------------------------------------
// ACTUALIZAR CONFIGURACIÓN
// -------------------------------------------------------------------------------------------------

void SmokeGuard::setSettings(const DetectionSettings& settings)
{
    /*
     * Actualizar solamente los parámetros del algoritmo.
     *
     * Las líneas base aprendidas se conservan para evitar reiniciar
     * innecesariamente el período de aprendizaje cada vez que se
     * modifica un umbral desde el panel de configuración.
     */
    this->settings = settings;
}


// -------------------------------------------------------------------------------------------------
// ANALIZAR DATOS DE LOS SENSORES
// -------------------------------------------------------------------------------------------------

SmokeAnalysis SmokeGuard::analyze(const SensorData& data)
{
    // ---------------------------------------------------------------------------------------------
    // COMPROBAR INICIALIZACIÓN
    // ---------------------------------------------------------------------------------------------

    if (!initialized)
    {
        SmokeAnalysis analysis;
        analysis.state = SmokeState::InsufficientData;
        return analysis;
    }


    const uint32_t currentTime = millis();


    // ---------------------------------------------------------------------------------------------
    // COMPROBAR INTERVALO ENTRE MUESTRAS
    // ---------------------------------------------------------------------------------------------

    /*
     * El intervalo de análisis permanece como una constante del firmware.
     *
     * No se considera un parámetro configurable porque el funcionamiento
     * de los sensores y algoritmos asociados depende de una cadencia
     * temporal estable.
     */
    if (lastSampleTime != 0 && currentTime - lastSampleTime < Config::SMOKEGUARD_SAMPLE_INTERVAL_MS)
    {
        return lastAnalysis;
    }

    lastSampleTime = currentTime;

    // ---------------------------------------------------------------------------------------------
    // COMPROBAR DATOS MÍNIMOS
    // ---------------------------------------------------------------------------------------------

    if (!hasMinimumData(data))
    {
        SmokeAnalysis analysis;
        analysis.state = SmokeState::InsufficientData;
        currentState = analysis.state;
        lastAnalysis = analysis;
        return analysis;
    }


    // ---------------------------------------------------------------------------------------------
    // PERÍODO DE APRENDIZAJE
    // ---------------------------------------------------------------------------------------------

    if (isWarmingUp())
    {
        /*
         * Durante el período inicial SmokeGuard aprende las condiciones
         * ambientales normales y todavía no genera una clasificación
         * de humo.
         */
        updateBaseline(data);


        SmokeAnalysis analysis;

        analysis.state = SmokeState::WarmingUp;
        analysis.fineParticleRatio = calculateFineParticleRatio(data);

        analysis.pm25Delta = data.pm2_5 - baselinePm2_5;

        analysis.vocDelta = static_cast<float>(data.vocIndex) - baselineVocIndex;

        if (data.sgp41NoxDataValid && baselineNoxSamples > 0)
        {
            analysis.noxDelta = static_cast<float>(data.noxIndex) - baselineNoxIndex;
        }

        analysis.humidityDelta = data.humidity - baselineHumidity;

        currentState = analysis.state;
        lastAnalysis = analysis;

        return analysis;
    }

    // ---------------------------------------------------------------------------------------------
    // CALCULAR EVIDENCIAS
    // ---------------------------------------------------------------------------------------------

    SmokeAnalysis analysis;

    analysis.score = calculateScore(data, analysis);

    // ---------------------------------------------------------------------------------------------
    // ACTUALIZAR PERSISTENCIA
    // ---------------------------------------------------------------------------------------------

    /*
     * Una muestra se considera sospechosa cuando alcanza al menos
     * la puntuación mínima configurada para el estado Suspicious.
     */
    const bool suspicious = analysis.score >= settings.scoreSuspicious;

    updatePersistence(suspicious);

    /*
     * La persistencia puede añadir puntuación justamente en esta muestra.
     *
     * Por ello se vuelve a calcular el resultado después de actualizar
     * el contador de muestras sospechosas.
     */
    analysis.score = calculateScore(data, analysis);

    // ---------------------------------------------------------------------------------------------
    // DETERMINAR ESTADO
    // ---------------------------------------------------------------------------------------------
    analysis.state = determineState(analysis.score);
    currentState = analysis.state;

    // ---------------------------------------------------------------------------------------------
    // ADAPTAR LÍNEA BASE
    // ---------------------------------------------------------------------------------------------

    /*
     * La línea base solamente se adapta cuando las condiciones actuales
     * siguen siendo consideradas normales.
     *
     * Esto evita que un evento de humo termine siendo incorporado
     * progresivamente a la línea base ambiental.
     */

    if (currentState == SmokeState::Normal)
    {
        updateBaseline(data);
    }

    lastAnalysis = analysis;

    return analysis;
}


// -------------------------------------------------------------------------------------------------
// REINICIAR SMOKEGUARD
// -------------------------------------------------------------------------------------------------

void SmokeGuard::reset()
{
    // ---------------------------------------------------------------------------------------------
    // ESTADO
    // ---------------------------------------------------------------------------------------------

    currentState = SmokeState::WarmingUp;

    startTime = millis();

    lastSampleTime = 0;

    baselineSamples = 0;

    baselineNoxSamples = 0;

    suspiciousSamples = 0;


    // ---------------------------------------------------------------------------------------------
    // LÍNEAS BASE
    // ---------------------------------------------------------------------------------------------

    baselinePm1_0 = 0.0f;

    baselinePm2_5 = 0.0f;

    baselinePm10 = 0.0f;

    baselineVocIndex = 0.0f;

    baselineNoxIndex = 0.0f;

    baselineTemperature = 0.0f;

    baselineHumidity = 0.0f;


    // ---------------------------------------------------------------------------------------------
    // ÚLTIMO ANÁLISIS
    // ---------------------------------------------------------------------------------------------

    lastAnalysis = SmokeAnalysis{};

    lastAnalysis.state = SmokeState::WarmingUp;
}


// -------------------------------------------------------------------------------------------------
// COMPROBAR DATOS MÍNIMOS
// -------------------------------------------------------------------------------------------------

bool SmokeGuard::hasMinimumData(const SensorData& data) const
{
    /*
     * El análisis necesita obligatoriamente:
     *
     * - Material particulado del SPS30.
     * - VOC del SGP41.
     * - Temperatura y humedad del SHT45.
     *
     * NOx no se considera obligatorio porque el SGP41 necesita un período
     * inicial de acondicionamiento antes de entregar datos NOx válidos.
     */
    return (data.sps30DataValid && data.sgp41VocDataValid && data.sht45DataValid);
}


// -------------------------------------------------------------------------------------------------
// COMPROBAR PERÍODO DE APRENDIZAJE
// -------------------------------------------------------------------------------------------------

bool SmokeGuard::isWarmingUp() const
{
    if (!initialized)
    {
        return true;
    }

    const uint32_t elapsedTime = millis() - startTime;

    /*
     * Deben cumplirse ambas condiciones para finalizar el aprendizaje:
     *
     * 1. Haber transcurrido el tiempo mínimo configurado.
     * 2. Haber recopilado la cantidad mínima de muestras.
     */
    return (
        elapsedTime < settings.baselineLearningMs ||
        baselineSamples < settings.minBaselineSamples
    );
}


// -------------------------------------------------------------------------------------------------
// ACTUALIZAR LÍNEA BASE
// -------------------------------------------------------------------------------------------------

void SmokeGuard::updateBaseline(const SensorData& data)
{
    const bool learning = isWarmingUp();

    // ---------------------------------------------------------------------------------------------
    // CONSTRUIR LÍNEA BASE INICIAL
    // ---------------------------------------------------------------------------------------------

    if (learning)
    {
        baselineSamples++;

        /*
         * Promedio incremental:
         *
         * promedioNuevo =
         * promedioAnterior + (valor - promedioAnterior) / cantidadMuestras
         *
         * Permite calcular el promedio sin almacenar todas las mediciones.
         */

        const float sampleCount =
            static_cast<float>(baselineSamples);

        baselinePm1_0 += (data.pm1_0 - baselinePm1_0) / sampleCount;

        baselinePm2_5 += (data.pm2_5 - baselinePm2_5) / sampleCount;

        baselinePm10 += (data.pm10 - baselinePm10) / sampleCount;

        baselineVocIndex += (static_cast<float>(data.vocIndex) - baselineVocIndex) / sampleCount;

        baselineTemperature += (data.temperature - baselineTemperature) / sampleCount;

        baselineHumidity += (data.humidity - baselineHumidity) / sampleCount;

        // -----------------------------------------------------------------------------------------
        // LÍNEA BASE NOx
        // -----------------------------------------------------------------------------------------

        /*
         * NOx utiliza su propio contador porque las primeras mediciones
         * del SGP41 todavía no son válidas durante el acondicionamiento.
         */
        if (data.sgp41NoxDataValid)
        {
            baselineNoxSamples++;

            const float noxSampleCount = static_cast<float>(baselineNoxSamples);

            baselineNoxIndex += (static_cast<float>(data.noxIndex) - baselineNoxIndex) / noxSampleCount;
        }

        return;
    }

    // ---------------------------------------------------------------------------------------------
    // ADAPTACIÓN LENTA DE LA LÍNEA BASE
    // ---------------------------------------------------------------------------------------------

    /*
     * Después del aprendizaje inicial se utiliza un promedio móvil
     * exponencial para adaptar lentamente la línea base a cambios
     * ambientales normales.
     */
    const float alpha = constrain(settings.baselineAlpha, 0.0f, 1.0f);

    baselinePm1_0 = baselinePm1_0 + alpha * (data.pm1_0 - baselinePm1_0);

    baselinePm2_5 = baselinePm2_5 + alpha * (data.pm2_5 - baselinePm2_5);

    baselinePm10 = baselinePm10 + alpha * (data.pm10 - baselinePm10);

    baselineVocIndex =
        baselineVocIndex +
        alpha *
        (
            static_cast<float>(data.vocIndex) -
            baselineVocIndex
        );


    baselineTemperature =
        baselineTemperature +
        alpha *
        (
            data.temperature -
            baselineTemperature
        );


    baselineHumidity =
        baselineHumidity +
        alpha *
        (
            data.humidity -
            baselineHumidity
        );


    // ---------------------------------------------------------------------------------------------
    // ADAPTAR LÍNEA BASE NOx
    // ---------------------------------------------------------------------------------------------

    if (data.sgp41NoxDataValid)
    {
        /*
         * Si por alguna razón todavía no existe una línea base NOx,
         * utilizar la primera lectura válida como punto inicial.
         */
        if (baselineNoxSamples == 0)
        {
            baselineNoxIndex =
                static_cast<float>(data.noxIndex);

            baselineNoxSamples = 1;
        }
        else
        {
            baselineNoxIndex =
                baselineNoxIndex +
                alpha *
                (
                    static_cast<float>(data.noxIndex) -
                    baselineNoxIndex
                );
        }
    }
}


// -------------------------------------------------------------------------------------------------
// CALCULAR RELACIÓN DE PARTÍCULAS FINAS
// -------------------------------------------------------------------------------------------------

float SmokeGuard::calculateFineParticleRatio(const SensorData& data) const
{
    if (data.pm2_5 <= 0.0f)
    {
        return 0.0f;
    }

    return data.pm1_0 / data.pm2_5;
}


// -------------------------------------------------------------------------------------------------
// CALCULAR PUNTUACIÓN
// -------------------------------------------------------------------------------------------------

uint8_t SmokeGuard::calculateScore(const SensorData& data, SmokeAnalysis& analysis) const
{
    uint16_t score = 0;


    // ---------------------------------------------------------------------------------------------
    // CALCULAR VARIACIONES
    // ---------------------------------------------------------------------------------------------

    analysis.pm25Delta = data.pm2_5 - baselinePm2_5;

    analysis.vocDelta = static_cast<float>(data.vocIndex) - baselineVocIndex;

    analysis.humidityDelta = data.humidity - baselineHumidity;

    analysis.fineParticleRatio = calculateFineParticleRatio(data);

    if (data.sgp41NoxDataValid && baselineNoxSamples > 0)
    {
        analysis.noxDelta = static_cast<float>(data.noxIndex) - baselineNoxIndex;
    }
    else
    {
        analysis.noxDelta = 0.0f;
    }

    // ---------------------------------------------------------------------------------------------
    // EVIDENCIA DE MATERIAL PARTICULADO
    // ---------------------------------------------------------------------------------------------

    analysis.particleEvidence = analysis.pm25Delta >= settings.pm25DeltaThreshold;

    if (analysis.particleEvidence)
    {
        score += settings.scoreParticles;
    }

    // ---------------------------------------------------------------------------------------------
    // EVIDENCIA DE PARTÍCULAS FINAS
    // ---------------------------------------------------------------------------------------------

    /*
     * La relación PM1.0 / PM2.5 solamente se considera evidencia
     * cuando también existe un incremento relevante de PM2.5.
     *
     * De esta manera una relación alta con una concentración de
     * partículas muy pequeña no genera puntuación por sí sola.
     */
    analysis.fineParticleEvidence =
        analysis.particleEvidence &&
        analysis.fineParticleRatio >=
        settings.fineParticleRatioThreshold;


    if (analysis.fineParticleEvidence)
    {
        score += settings.scoreFineParticles;
    }

    // ---------------------------------------------------------------------------------------------
    // EVIDENCIA VOC
    // ---------------------------------------------------------------------------------------------

    analysis.vocEvidence = analysis.vocDelta >= settings.vocDeltaThreshold;

    if (analysis.vocEvidence)
    {
        score += settings.scoreVoc;
    }

    // ---------------------------------------------------------------------------------------------
    // EVIDENCIA NOx
    // ---------------------------------------------------------------------------------------------

    analysis.noxEvidence =
        data.sgp41NoxDataValid &&
        baselineNoxSamples > 0 &&
        analysis.noxDelta >=
        settings.noxDeltaThreshold;

    if (analysis.noxEvidence)
    {
        score += settings.scoreNox;
    }

    // ---------------------------------------------------------------------------------------------
    // EVIDENCIA DE PERSISTENCIA
    // ---------------------------------------------------------------------------------------------

    analysis.persistenceEvidence =
        settings.persistenceSamples > 0 &&
        suspiciousSamples >=
        settings.persistenceSamples;

    if (analysis.persistenceEvidence)
    {
        score += settings.scorePersistence;
    }

    // La puntuación final siempre debe permanecer entre 0 y 100.
    if (score > 100)
    {
        score = 100;
    }

    return static_cast<uint8_t>(score);
}

// -------------------------------------------------------------------------------------------------
// ACTUALIZAR PERSISTENCIA
// -------------------------------------------------------------------------------------------------

void SmokeGuard::updatePersistence(bool suspicious)
{
    if (!suspicious)
    {
        suspiciousSamples = 0;
        return;
    }

    /*
     * Evitar un desbordamiento del contador si el dispositivo permanece
     * durante mucho tiempo en condiciones sospechosas.
     */
    if (suspiciousSamples < UINT16_MAX)
    {
        suspiciousSamples++;
    }
}

// -------------------------------------------------------------------------------------------------
// DETERMINAR ESTADO
// -------------------------------------------------------------------------------------------------

SmokeState SmokeGuard::determineState(uint8_t score) const
{
    if (score >= settings.scoreHighConfidence)
    {
        return SmokeState::SmokeHighConfidence;
    }

    if (score >= settings.scoreProbable)
    {
        return SmokeState::SmokeProbable;
    }

    if (score >= settings.scoreSuspicious)
    {
        return SmokeState::Suspicious;
    }

    return SmokeState::Normal;
}