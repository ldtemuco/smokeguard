#include "Sps30Sensor.h"
#include "Config.h"
// =================================================================================================

// -------------------------------------------------------------------------------------------------
// INICIAR SENSOR SPS30
// -------------------------------------------------------------------------------------------------

bool Sps30Sensor::begin(TwoWire& bus) 
{

    bus.beginTransmission(Config::SPS30_ADDRESS);

    if (bus.endTransmission() != 0) 
    {
        Serial0.println("[ERROR] SPS30 no detectado.");
        available = false;
        return false;
    }

    sensor.begin(bus, Config::SPS30_ADDRESS);

    sensor.stopMeasurement();

    delay(100);

    const int16_t error = sensor.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_FLOAT);

    if (error != 0) 
    {
        Serial0.printf("[ERROR] SPS30 inicio fallido: %d\n", error);
        available = false;
        return false;
    }

    available = true;

    Serial0.println("[OK] SPS30 inicializado.");

    return true;
}

// -------------------------------------------------------------------------------------------------
// LEER DATOS DE SENSOR SPS30
// -------------------------------------------------------------------------------------------------

bool Sps30Sensor::read(SensorData& data) 
{
    if (!available) 
    {
        data.sps30DataValid = false;
        return false;
    }

    uint16_t dataReady = 0;

    const int16_t readyError = sensor.readDataReadyFlag(dataReady);

    if (readyError != 0) 
    {
        data.sps30DataValid = false;
        return false;
    }

    // No hay una medición nueva todavía. Conservamos la anterior.  

    if (dataReady != 1) 
    {
        return false;
    }

    // Concentración de masa de partículas (µg/m³).
    float pm1_0 = 0.0f;   // Partículas de hasta 1.0 µm.
    float pm2_5 = 0.0f;   // Partículas de hasta 2.5 µm.
    float pm4_0 = 0.0f;   // Partículas de hasta 4.0 µm.
    float pm10 = 0.0f;    // Partículas de hasta 10 µm.

    // Concentración numérica de partículas.
    float nc0_5 = 0.0f;   // Cantidad de partículas de hasta 0.5 µm.
    float nc1_0 = 0.0f;   // Cantidad de partículas de hasta 1.0 µm.
    float nc2_5 = 0.0f;   // Cantidad de partículas de hasta 2.5 µm.
    float nc4_0 = 0.0f;   // Cantidad de partículas de hasta 4.0 µm.
    float nc10 = 0.0f;    // Cantidad de partículas de hasta 10 µm.

    // Tamaño medio estimado de las partículas detectadas (µm).
    float particleSize = 0.0f;


    const int16_t error =
        sensor.readMeasurementValuesFloat(
            pm1_0,
            pm2_5,
            pm4_0,
            pm10,
            nc0_5,
            nc1_0,
            nc2_5,
            nc4_0,
            nc10,
            particleSize
        );

    if (error != 0) 
    {
        data.sps30DataValid = false;
        return false;
    }

    data.pm1_0 = pm1_0;
    data.pm2_5 = pm2_5;
    data.pm4_0 = pm4_0;
    data.pm10 = pm10;

    data.nc0_5 = nc0_5;
    data.nc1_0 = nc1_0;
    data.nc2_5 = nc2_5;
    data.nc4_0 = nc4_0;
    data.nc10 = nc10;

    data.particleSize = particleSize;

    data.sps30DataValid = true;

    return true;
}