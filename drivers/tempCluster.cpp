/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h>

#include "tempCluster.h"
#include <math.h> // for round


static const char *TEMP_TAG = "TempCluster";


// Event handler for periodic soft task
void TempCluster::readTemperature()
{
    float temperature = 0;
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(_tHandle, &temperature));

    int16_t attr = (int16_t)(round(temperature * 100)); // cluster shall be in Celsius * 100

    ESP_LOGV(ZCLUSTER_TAG, "TempCluster - temperature: %f°C - attr: %d", 
                temperature, attr);

    setTemperatureMeasuredValue(attr);
}


// Private constructor
TempCluster::TempCluster(): ZbTemperatureMeasCluster(false, -1000, 5000), 
            _tHandle(NULL)
{
    setup();
}

void TempCluster::start(uint64_t delay_ms)
{
    ESP_ERROR_CHECK(temperature_sensor_enable(_tHandle));
    _periodicTask = new PeriodicSoftTask(&TempCluster::readTemperature, 
                            this, delay_ms, "temperature");
    
}

void TempCluster::stop(void)
{
    delete _periodicTask;
    _periodicTask = nullptr;
    ESP_ERROR_CHECK(temperature_sensor_disable(_tHandle));
}

void TempCluster::setup(void)
{
    ESP_LOGV(TEMP_TAG, "Configuration Temperature Driver --------------");

    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 50);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &_tHandle));
    
}

void TempCluster::setTemperatureMeasuredValue(int16_t newValue)
{
    setAttribute(ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                &newValue);
}
