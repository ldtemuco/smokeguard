#include "Sht45Sensor.h"
#include "Config.h"
// =================================================================================================

// -------------------------------------------------------------------------------------------------
// INICIAR SENSOR SHT45
// -------------------------------------------------------------------------------------------------

bool Sht45Sensor::begin(TwoWire& bus) 
{

    // Comprobar que el dispositivo responde en el bus I2C.
    bus.beginTransmission(Config::SHT45_ADDRESS);

    if (bus.endTransmission() != 0) 
    {

        Serial0.println("[ERROR] SHT45 no detectado.");
        available = false;
        return false;
    }

    sensor.begin(bus, Config::SHT45_ADDRESS);

    sensor.softReset();

    delay(10);

    available = true;

    Serial0.println("[OK] SHT45 inicializado.");

    return true;
}

// -------------------------------------------------------------------------------------------------
// LEER DATOS DE SENSOR SHT45
// -------------------------------------------------------------------------------------------------

bool Sht45Sensor::read(SensorData& data) 
{

    if (!available) 
    {
        data.sht45DataValid = false;
        return false;
    }

    float temperature = 0.0f;
    float humidity = 0.0f;

    const int16_t error = sensor.measureHighPrecision(temperature, humidity);


    if (error != 0) 
    {

        data.sht45DataValid = false;
        return false;
    }


    data.temperature = temperature;
    data.humidity = humidity;

    data.sht45DataValid = true;

    return true;
}