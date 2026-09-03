#pragma once
// =================================================================================================
#include <Arduino.h>
// =================================================================================================
/*
* GLOSARIO
* --------------------------------------------------------------------------------------------------
* SHT45 = Sensor de Humedad y Temperatura Sensirion.
*
* SGP41 = Sensor de Gases Sensirion.
*
* SPS30 = Sensor de Partículas Sensirion.
*
* SRAW  = Signal Raw | Señal Cruda.
* 
* VOC   = Volatile Organic Compounds | Compuestos Orgánicos Volátiles.
* 
* NOx   = Nitrogen Oxides | Óxidos de Nitrógeno.
* 
* PM    = Particulate Matter | Material Particulado 
*         Tamaño de partículas por unidad de volumen.
*
* NC    = Number Concentration | Concentración Numérica 
*         Número de partículas por unidad de volumen.
*
* µg/m³ = Microgramos por metro cúbico.
*
* µm    = Micrómetros.
*/

/**
 * @struct SensorData 
 * @brief Estructura para almacenar los datos de los sensores. 
**/
struct SensorData 
{
    // =============================================================================================
    // SHT45
    // =============================================================================================
    
    /**
     * @brief Indica si la última lectura del sensor SHT45 fue válida.
    */   
    bool  sht45DataValid = false; 

    /**
     * @brief Almacena la última lectura de temperatura del sensor SHT45.
    */    
    float temperature = 0.0f;
    
    /**
     * @brief Almacena la última lectura de humedad del sensor SHT45.
    */
    float humidity = 0.0f;

    // =============================================================================================
    // SGP41
    // =============================================================================================

    // Indica si la última lectura de VOC del sensor SGP41 fue válida.
    bool sgp41VocDataValid = false;

    // Indica si la última lectura de NOx del sensor SGP41 fue válida.
    bool sgp41NoxDataValid = false;

  
    // Cantidad de compuestos orgánicos volátiles medidos en la útlima lectura.
    uint16_t srawVoc = 0;
    // Cantidad de compuestos óxido de nitrogeno medidos la útlima lectura.
    uint16_t srawNox = 0;

    // Índice calculado por el algoritmo de Sensirion a partir de la señal cruda de VOC.
    int32_t vocIndex = 0;
    // Índice calculado por el algoritmo de Sensirion a partir de la señal cruda de Nox.
    int32_t noxIndex = 0;

    // =============================================================================================
    // SPS30
    // =============================================================================================
    
    // Indica si la última lectura del sensor SPS30 fue válida.
    bool sps30DataValid = false;

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
};