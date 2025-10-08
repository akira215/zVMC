/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "driver/temperature_sensor.h"
#include "ZbTemperatureMeasCluster.h"
#include "periodicSoftTask.h"

class TempCluster: public ZbTemperatureMeasCluster
{

public:
    TempCluster();

    void setup(void);

    /// @brief Start periodic query of the sensors 
    /// @param delay_ms Delay between 2 queries, should be greater than query duration (Sum of each conversion times)
    void start(uint64_t delay_ms = 1000);
    
    
    void stop(void);
    
    /// @brief Event handler when conversion is received
    static void ads1115_event_handler(uint16_t input, double value);

    /// @brief Event handler for periodic task
    /// @brief read the temperature
    void readTemperature();

private:
    PeriodicSoftTask* _periodicTask;
    temperature_sensor_handle_t _tHandle;

    void setTemperatureMeasuredValue(int16_t newValue);

}; // TempCluster Class