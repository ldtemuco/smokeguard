#include "SensorManager.h"
#include "config.h"
// =================================================================================================

// -------------------------------------------------------------------------------------------------
// INICIAR ADMINISTRADOR DE SENSORES
// -------------------------------------------------------------------------------------------------

bool SensorManager::begin()
{
    // ---------------------------------------------------------------------------------------------
    // INICIAR BUS I2C PRINCIPAL
    // SHT45 + SGP41
    // ---------------------------------------------------------------------------------------------

    Wire.begin(
        Config::SENSOR_SDA,
        Config::SENSOR_SCL,
        Config::I2C_FREQUENCY
    );

    Wire.setTimeOut(Config::I2C_TIMEOUT_MS);


    // ---------------------------------------------------------------------------------------------
    // INICIAR BUS I2C SECUNDARIO
    // SPS30
    // ---------------------------------------------------------------------------------------------

    Wire1.begin(
        Config::SPS30_SDA,
        Config::SPS30_SCL,
        Config::I2C_FREQUENCY
    );

    Wire1.setTimeOut(Config::I2C_TIMEOUT_MS);


    // ---------------------------------------------------------------------------------------------
    // INICIAR SENSORES
    // ---------------------------------------------------------------------------------------------

    const bool sht45Available = sht45.begin(Wire);

    const bool sgp41Available = sgp41.begin(Wire);

    const bool sps30Available = sps30.begin(Wire1);


    // Reiniciar temporizador de lectura.
    lastUpdateMillis = millis();


    // La inicialización se considera correcta solamente si todos los sensores respondieron.
    return (
        sht45Available &&
        sgp41Available &&
        sps30Available
    );
}


// -------------------------------------------------------------------------------------------------
// ACTUALIZAR DATOS DE LOS SENSORES
// -------------------------------------------------------------------------------------------------

void SensorManager::update()
{
    const uint32_t currentMillis = millis();


    // Comprobar si se cumplió el intervalo de lectura.
    if (currentMillis - lastUpdateMillis < Config::SENSOR_INTERVAL_MS)
    {
        return;
    }


    lastUpdateMillis = currentMillis;


    /*
     * ORDEN DE LECTURA
     * ---------------------------------------------------------------------------------------------
     * El SHT45 debe leerse antes que el SGP41 porque los valores de temperatura y humedad se
     * utilizan para compensar las mediciones de VOC y NOx.
     */

    sht45.read(data);

    sgp41.read(data);

    sps30.read(data);
}


// -------------------------------------------------------------------------------------------------
// OBTENER DATOS DE LOS SENSORES
// -------------------------------------------------------------------------------------------------

const SensorData& SensorManager::getData() const
{
    return data;
}