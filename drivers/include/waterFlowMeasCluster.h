/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/
#pragma once

#include "cppgpio.h"
#include "persistedValue.h"
#include "scheduledTask.h"
#include "ZbFlowMeasCluster.h"
#include "zbAnalogValueCluster.h"

#define K_FACTOR_ATTR                   "\x07""Kfactor" //TODO del
#define GPIO_VOLUME_METER               20  //!< GPIO number connect to the Meter


/// @brief Driver to count flow from impulsion Meter - active low
class WaterFlowMeasCluster : public ZbFlowMeasCluster
{
    GpioInput _irqMeter     { static_cast<gpio_num_t>(GPIO_VOLUME_METER) };
    PersistedValue<float_t> _Kfactor;
    ZbAnalogValueCluster*   _kfactorCluster = nullptr;
    //PeriodicSoftTask*       _reportTask = nullptr;
    uint16_t                _pulseCount; // pulse counter re
    uint64_t                _currentVolume = 0; // In fact it is not the current volume but number of tick
    uint64_t                _secondsFromMidnight = 0;
    ScheduledTask*          _resetTask   = nullptr; 
public:
    WaterFlowMeasCluster();

    /// @brief set the k factor of the volumetric meter (write to NVS)
    /// @param kFactor the factor (unit shall be the same as current volume)
    void setKfactor(clusterEvent_t event, std::vector<attribute_t> attrs);

    /// @brief return a pointer to the embedded kfactor cluster
    ZbCluster* getKfactorCluster();
    
    /// @brief setup a task to reset the counter to 0 at a given time of the day
    void setupResetTask(uint64_t secondsFromMidnight = 0);


private:
    // Event Handler for GPIO that is triggered by tick counter
    static void impulsionHandler(void *handler_args, esp_event_base_t base,
                                 int32_t id, void *event_data);

    // Set the attribute value for the cluster
    void setFlowMeasuredValue(uint16_t newValue);
    
    /// @brief Reset counter to 0
    void resetCounter();

};