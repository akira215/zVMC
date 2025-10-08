/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/
#pragma once

#include "persistedValue.h"
#include "ZbPressureMeasCluster.h"
#include "zbAnalogValueCluster.h"

/// @brief Driver to read pressure from ADS1115 analog converter
class WaterPressureMeasCluster : public ZbPressureMeasCluster
{
    uint8_t                 _channel;
    PersistedValue<float_t> _Kfactor;
    ZbAnalogValueCluster*   _kfactorCluster = nullptr;
public:

    /// @brief Pressure Measurement cluster constructor
    /// @param channel of the ads1115 AD converter for this 
    WaterPressureMeasCluster(uint8_t channel);

    /// @brief set the k factor of this sensor
    /// @param kFactor the factor (unit shall be the same as current volume)
    void setKfactor(clusterEvent_t event, std::vector<attribute_t> attrs);

    /// @brief return a pointer to the embedded kfactor cluster
    ZbCluster* getKfactorCluster();

    /// @brief Event handler when conversion is received
    void adc_event_handler(double value);

private:
    // Set the attribute value for the cluster
    void setPressureMeasuredValue(int16_t newValue);

};