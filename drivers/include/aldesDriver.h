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

struct AldesData_t
{
    uint32_t    product_code       = 0xffffffff;
    uint64_t    serial_num         = 0xffffffffffffffff;
    uint16_t    firm_ver           = 0xffff;

    uint16_t    regulation_mode   = 0xffff; //0:airflow /1:Hydro/2:Speed 
    uint16_t    demand_user       = 0xffff; //1:Daily/2:Boost/3:Invités/0:Holidays/255:Ignorer/4:MaxSpeed(DK)
    uint16_t    demand_programmer = 0xffff; //1:Daily/2:Boost/3:Invités/0:Holidays/4:MaxSpeed(DK)
    uint16_t    bypass_mode       = 0xffff; //0:Manual/1:Auto 

    uint16_t    tempo_filter       = 0xffff; //month

    int16_t     T_intake_air_out   = 0xffff; // 0.01 °C
    int16_t     T_extract_air_in   = 0xffff; // 0.01 °C
    int16_t     T_supply_air_in    = 0xffff; // 0.01 °C
    int16_t     T_exhaust_air_out  = 0xffff; // 0.01 °C

    uint16_t    speed_exhaust_fan  = 0xffff; // rpm
    uint16_t    speed_supply_fan   = 0xffff; // rpm
    uint16_t    airflow_MVE        = 0xffff; // m3/h
    uint16_t    airflow_MVI        = 0xffff; // m3/h

    int16_t     pressure           = 0xffff; // 0.1 Pa
    uint16_t    bypass_position     = 0xffff; // "0:Not defined/1:Open/2:closed 45°/3:closed"

    uint16_t    setting_MVE_vacation   = 0xffff; //m3/h
    uint16_t    setting_MVI_vacation   = 0xffff; //m3/h
    uint16_t    setting_MVE_daily      = 0xffff; //m3/h
    uint16_t    setting_MVI_daily      = 0xffff; //m3/h
    uint16_t    setting_MVE_pushButton = 0xffff; //m3/h
    uint16_t    setting_MVI_pushButton = 0xffff; //m3/h
    uint16_t    setting_MVE_boost      = 0xffff; //m3/h
    uint16_t    setting_MVI_boost      = 0xffff; //m3/h
    uint16_t    setting_MVE_maxSpeed   = 0xffff; //m3/h
    uint16_t    setting_MVI_maxSpeed   = 0xffff; //m3/h
};

class AldesDriver final
{

public:
    AldesDriver();

    /// @brief Start periodic query of the sensors 
    /// @param delay_ms Delay between 2 queries, should be greater than query duration (Sum of each conversion times)
    void start(uint64_t delay_ms = 1000);
    
    /// @brief Start periodic queries
    void stop(void);
    
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
    void getDeviceInfos();
    void getSpeedSettings();
    void getRegulationParams();

    void getFilterTimerState();
    void getBypassPosition();
    void getTemperaturesAndFanSpeed();

    void setUserLevel(uint8_t lvl);
    void postEvent(uint8_t channel, double value);

    static const char* aldesDeviceFromCode(uint32_t code);
    
private:

    AldesData_t _data;

    /// @brief Callback type, only one call back for each channel
    typedef std::function<void(double)> adsCallback_t;
    
    // Map of call back first is channel, second is callback
    std::map<uint8_t, adsCallback_t> _adsCallbacks;

    PeriodicSoftTask* _periodicTask;
    ModbusMaster* _mb_master = nullptr;
      

}; // AldesDriver Class