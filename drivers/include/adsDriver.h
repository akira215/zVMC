/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include <functional>
#include <map>

#include "cppgpio.h"
#include "cppi2c.h"
#include "ads1115.h"
#include "periodicSoftTask.h"

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL      //!< GPIO number used for I2C master clock 
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA      //!< GPIO number used for I2C master data  
#define I2C_MASTER_NUM              I2C_NUM_0                  //!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip 
#define ADS_I2C_FREQ_HZ             400000                     //!< I2C master clock frequency 
#define ADS_I2C_TIMEOUT_MS          1000
#define GPIO_INPUT_IO_READY         CONFIG_ADS1115_READY_INT   //!< GPIO number connect to the ready pin of the converter

// Singleton class
class AdsDriver final
{
    // Private constructor for singleton
    AdsDriver();
public:
    static AdsDriver& getInstance();

    // Avoid copy constructors for singleton
    AdsDriver(AdsDriver const&)       = delete;
    void operator=(AdsDriver const&)  = delete;
    
    
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
    /// @brief Trigger the conversion
    void trigger_conversion();

    /// @brief register event handler for this cluster.
    /// to pass args to the function, use std::bind
    /// @param func pointer to the method ex: &Main::clusterHandler
    /// @param instance instance of the object for this handler (ex: this)
    /// @param channel of the ads converter that will trigger this 
    template<typename C>
    void registerAdsHandler(void (C::* func)(double), C* instance, uint8_t channel) {
        _adsCallbacks.insert({channel, std::bind(func,std::ref(*instance),std::placeholders::_1)}); 
    }
    
private:
    I2c _i2c_master;
    Ads1115 _ads;
    //double _voltage[4];
    TickType_t _tickToWait;
    PeriodicSoftTask* _periodicTask;

    double _voltage[4];

    /// @brief Callback type, only one call back for each channel
    typedef std::function<void(double)> adsCallback_t;
    
    // Map of call back first is channel, second is callback
    std::map<uint8_t, adsCallback_t> _adsCallbacks;

    void postEvent(uint8_t channel, double value);
      

}; // adsDriver Class