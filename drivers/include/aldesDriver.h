/*
  zVMC 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include <functional>
#include <map>

#include "modbusMaster.h" 
#include "periodicSoftTask.h"


// Singleton class
class AldesDriver final
{

public:
    AldesDriver();

    void setup(void);

    /// @brief Start periodic query of the sensors 
    /// @param delay_ms Delay between 2 queries, should be greater than query duration (Sum of each conversion times)
    void start(uint64_t delay_ms = 1000);
    
    
    void stop(void);
    
/*
    /// @brief Set the voltage of the input
    /// @param  channel between 0 and 3, correspondint voltage
    /// @param value value to be set
    void setVoltage(uint8_t channel, double value);

    /// @brief Get the current voltage of the input
    /// @param  channel between 0 and 3, correspondint voltage
    /// @return the voltage for this input
    double getVoltage(uint8_t  channel);
*/

    /// @brief Event handler when conversion is received
    static void ads1115_event_handler(uint16_t input, double value);

    /// @brief Event handler for periodic task
    /// @brief Trigger a reading on the device
    void trigger_reading();

    /// @brief register event handler for this cluster.
    /// to pass args to the function, use std::bind
    /// @param func pointer to the method ex: &Main::clusterHandler
    /// @param instance instance of the object for this handler (ex: this)
    /// @param channel of the ads converter that will trigger this 
    template<typename C>
    void registerAldesHandler(void (C::* func)(double), C* instance, uint8_t channel) {
        _adsCallbacks.insert({channel, std::bind(func,std::ref(*instance),std::placeholders::_1)}); 
    }
    
private:
    ModbusMaster* _mb_master = nullptr;

    PeriodicSoftTask* _periodicTask;

    double _voltage[4];

    /// @brief Callback type, only one call back for each channel
    typedef std::function<void(double)> adsCallback_t;
    
    // Map of call back first is channel, second is callback
    std::map<uint8_t, adsCallback_t> _adsCallbacks;

    void postEvent(uint8_t channel, double value);
      

}; // AldesDriver Class