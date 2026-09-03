#include "Sgp41Sensor.h"
#include "Config.h"
// =================================================================================================

// -------------------------------------------------------------------------------------------------
// INICIAR SENSOR SGP41
// -------------------------------------------------------------------------------------------------

bool Sgp41Sensor::begin(TwoWire& bus)
{

    // Comprobar que el dispositivo responde en el bus I2C.
    bus.beginTransmission(Config::SGP41_ADDRESS);

    if (bus.endTransmission() != 0) 
    {
        Serial0.println("[ERROR] SGP41 no detectado.");
        available = false;
        return false;
    }

    // Inicializar sensor.
    sensor.begin(bus);

    // Reiniciar los algoritmos de índice.
    vocAlgorithm.reset();
    noxAlgorithm.reset();

    // Reiniciar período de acondicionamiento NOx.
    conditioningSeconds = Config::SGP41_CONDITIONING_SECONDS;

    available = true;

    Serial0.println("[OK] SGP41 inicializado.");

    return true;
}

// -------------------------------------------------------------------------------------------------
// LEER DATOS DE SENSOR SGP41
// -------------------------------------------------------------------------------------------------

bool Sgp41Sensor::read(SensorData& data) 
{

    if (!available) 
    {
        data.sgp41VocDataValid = false;
        data.sgp41NoxDataValid = false;
        return false;
    }

    /*
     * COMPENSACIÓN DE TEMPERATURA Y HUMEDAD
     * ---------------------------------------------------------------------------------------------
     * Valores predeterminados recomendados para el SGP41:
     *
     * 0x8000 = 50% de humedad relativa.
     * 0x6666 = 25°C.
     *
     * Se utilizan solamente si no existe una lectura válida del SHT45.
     */

    uint16_t humidityTicks = 0x8000; // 50% de humedad relativa.
    uint16_t temperatureTicks = 0x6666; // 25°C de temperatura.

    if (data.sht45DataValid) 
    {
        humidityTicks = humidityToTicks(data.humidity);
        temperatureTicks = temperatureToTicks(data.temperature);
    }

    uint16_t srawVoc = 0;
    uint16_t srawNox = 0;
    uint16_t error = 0;

    // ---------------------------------------------------------------------------------------------
    // ACONDICIONAMIENTO DEL SENSOR NOx
    // ---------------------------------------------------------------------------------------------
  
    if (conditioningSeconds > 0) 
    {

        /*
         * Durante los primeros 10 segundos el SGP41 acondiciona el canal NOx.
         *
         * Durante este período solamente se obtiene una señal SRAW VOC válida.
         */

        error = sensor.executeConditioning(humidityTicks, temperatureTicks, srawVoc);


        if (error != 0) 
        {
            data.sgp41VocDataValid = false;
            data.sgp41NoxDataValid = false;

            Serial0.printf("[ERROR] SGP41 acondicionamiento fallido. Código: %u\n", error);

            return false;
        }

        conditioningSeconds--;

        // Guardar señal cruda VOC.
        data.srawVoc = srawVoc;

        // Durante el acondicionamiento SRAW NOx permanece en 0.
        data.srawNox = 0;

        // El algoritmo VOC puede procesarse desde el inicio.        
        data.vocIndex =vocAlgorithm.process(srawVoc);

        /*
         * El algoritmo NOx debe seguir ejecutándose a intervalos regulares de 1 segundo.
         * Durante el acondicionamiento recibe 0.
         */

        data.noxIndex = noxAlgorithm.process(0);
        data.sgp41VocDataValid = true;
        data.sgp41NoxDataValid = false;

        return true;
    }

    // ---------------------------------------------------------------------------------------------
    // MEDICIÓN NORMAL DEL SENSOR SGP41
    // ---------------------------------------------------------------------------------------------

    error = sensor.measureRawSignals(humidityTicks, temperatureTicks, srawVoc, srawNox);

    if (error != 0) 
    {
        data.sgp41VocDataValid = false;
        data.sgp41NoxDataValid = false;

        Serial0.printf("[ERROR] SGP41 lectura fallida. Código: %u\n", error);

        return false;
    }

    // ---------------------------------------------------------------------------------------------
    // GUARDAR SEÑALES CRUDAS
    // ---------------------------------------------------------------------------------------------

    data.srawVoc = srawVoc;
    data.srawNox = srawNox;

    // ---------------------------------------------------------------------------------------------
    // CALCULAR ÍNDICES
    // ---------------------------------------------------------------------------------------------

    data.vocIndex = vocAlgorithm.process(srawVoc);

    data.noxIndex = noxAlgorithm.process(srawNox);

    // Ambas mediciones son válidas.
    data.sgp41VocDataValid = true;
    data.sgp41NoxDataValid = true;

    return true;
}


// -------------------------------------------------------------------------------------------------
// CONVERSIÓN DE HUMEDAD
// -------------------------------------------------------------------------------------------------

uint16_t Sgp41Sensor::humidityToTicks(float humidity) 
{
    // El SGP41 acepta una humedad relativa entre el 0% y el 100%.   

    if (humidity < 0.0f) 
    {
        humidity = 0.0f;
    }

    if (humidity > 100.0f) 
    {
        humidity = 100.0f;
    }

    return static_cast<uint16_t>( ( humidity * 65535.0f ) / 100.0f );       
    
}

// -------------------------------------------------------------------------------------------------
// CONVERSIÓN DE TEMPERATURA
// -------------------------------------------------------------------------------------------------

uint16_t Sgp41Sensor::temperatureToTicks(float temperature) 
{

    // El SGP41 acepta un rango de temperatura entre -45°C y 130°C.
    
    if (temperature < -45.0f) 
    {
        temperature = -45.0f;
    }

    if (temperature > 130.0f) 
    {
        temperature = 130.0f;
    }

    return static_cast<uint16_t>( ( (temperature + 45.0f) * 65535.0f ) / 175.0f );
}