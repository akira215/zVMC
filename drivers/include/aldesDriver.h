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
#include "zbCluster.h"      // require for callbacks on GUI change

class AldesDriver final
{
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
        
        uint16_t    filter_state_percent    = 0xffff; //%
        uint16_t    filter_state_days       = 0xffff; //elapsed hours since reset

        int16_t     T_intake_air_out   = 0xffff; // 0.01 °C
        int16_t     T_extract_air_in   = 0xffff; // 0.01 °C
        int16_t     T_supply_air_in    = 0xffff; // 0.01 °C
        int16_t     T_exhaust_air_out  = 0xffff; // 0.01 °C

        uint16_t    bypass_position    = 0xffff; // "0:Not defined/1:Open/2:closed 45°/3:closed"

        uint16_t    speed_exhaust_fan  = 0xffff; // rpm
        uint16_t    speed_supply_fan   = 0xffff; // rpm
        uint16_t    airflow_MVE        = 0xffff; // m3/h
        uint16_t    airflow_MVI        = 0xffff; // m3/h

        int16_t     pressure           = 0xffff; // 0.1 Pa

        uint16_t    season_detection    = 0xffff; // 0:Unknown/1:Winter/2:Summer
        uint16_t    error_code          = 0xffff; // 0:NoError/49:NoProductId/50:NoMiseEnService/53:Défaut Capteur Pression/70:Contact Sec Actif/72:Défaut Sonde HR/76:Défaut Sonde Co2 IHM/81:BCA Absente/82:Echec BCA/83:Défaut BCA PréChauf. Ext./84:Défaut PréChauffage Int./85:Défaut BCA PostChauf. Ext./90:Echec Test FireDamper/91:FireDamper Fermé/182:Défaillance MVE/183:Défaillance MVI/239:Défaut sonde Rejet/240:Défaut sonde Extérieur/241:Défaut sonde Insufflation/251:Défaut Sonde VMC
        
        int16_t     T_bypass_summer     = 0xffff; // 0.01°C between 19 & 28

        uint16_t    fan_config          = 0xffff; //2:Config A/1:Config B/0:None
        
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

        uint16_t    current_level           = 0xffff; //1:Daily/2:Boost/3:Invités/0:Holidays/4:MaxSpeed(DK)
        uint16_t    requester               = 0xffff; //0:User/1:Prog IOT/2:Capteur IOT/3:Régulation/4:Option HR/5:Entrée AI0/6:Entrée AI1/7:Bouton Cuisine/8:Co2 IHM/9:Mode SAFE/10:Air Prog

        uint16_t    date_time               = 0xffffffff; // numer of second since 01/01/2000 00:00:00

    };

public:
    AldesDriver();
    ~AldesDriver();

    /// @brief Query device for all global infos
    /// @return ture if succeed, false otherwise
    void query_global_infos();

    /// @brief Start periodic query of the sensors 
    /// @param delay_ms Delay between 2 queries, should be greater than query duration (Sum of each conversion times)
    void start(uint64_t poll_fast_ms = 1000, uint64_t poll_slow_s = 20);
    
    /// @brief Start periodic queries
    void stop(void);
    
    /// @brief Event handler for periodic task
    /// @brief Trigger a reading on the device
    void query_device_fast();

    /// @brief Event handler for periodic task
    /// @brief Trigger a reading on the device for filters, ...
    void query_device_slow();

    /// @brief Triggered when remote change a attribute
    /// @brief will update the Aldes device via modbus
    void onFilterChange(ZbCluster::clusterEvent_t event, 
                std::vector<ZbCluster::attribute_t> attrs);

    /// @brief register event handler for this driver
    /// to pass args to the function, use std::bind
    /// @param func pointer to the method ex: &Main::clusterHandler
    /// @param instance instance of the object for this handler (ex: this)
    /// @param channel of the ads converter that will trigger this 
    template<typename C>
    void registerAldesHandler(void (C::* func)(int16_t), C* instance, uint16_t channel) {
        _aldesCallbacks.insert({channel, std::bind(func,std::ref(*instance),std::placeholders::_1)}); 
    }

    void setFilterTimer (uint16_t hours);
    void setFilterTempo (uint16_t months);

    void setBypassTemperature (int16_t temperature);
    void setDemandPoint (uint16_t mode);

    void setVacationLevel (uint16_t flowrate);

private:
    bool getDeviceInfos();
    bool getSpeedSettings();
    bool getRegulationParams();
    bool getFanConfig();

    void getFilterTimerState();
    void getSeasonDetection();
    void getDate();
    void getErrorCode();

    void getTBypassSummer();

    void getCurrentState();
    
    void getBypassPosition();
    void getTemperaturesAndFanSpeed();

    void setUserLevel(uint8_t lvl);


    // All data will be sent as int16_t, cast to uint16_t may be required,
    // depending on actual value type
    void postEvent(uint16_t channel, int16_t value);

    static const char* aldesDeviceFromCode(uint32_t code);
    
private:

    AldesData_t _data;

    /// @brief Callback type, only one call back for each channel
    typedef std::function<void(int16_t)> aldesCallback_t;
    
    // Map of call back first is channel, second is callback
    std::map<uint16_t, aldesCallback_t> _aldesCallbacks;

    PeriodicSoftTask* _fastPollTask = nullptr;
    PeriodicSoftTask* _slowPollTask = nullptr;
    ModbusMaster*    _mb_master = nullptr;

}; // AldesDriver Class