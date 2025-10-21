/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h> // TODEL development purpose

#include "aldesDriver.h"
#include "aldesModbus.h"
#include "zbNode.h"

static const char *ALDES_TAG = "AldesDriver";


// Static event handler
/*
void AldesDriver::ads1115_event_handler(uint16_t input, double value)
{
    ESP_LOGV(ALDES_TAG, "Callback Main Ads1115 input: %d - value: %f", input-4, value);

    // Post event to update the attribute of concerning registered clusters
    //AldesDriver::getInstance().postEvent(input-4, value);

}
    */

// Event handler for periodic soft task
void AldesDriver::trigger_reading()
{
   
    getBypassPosition();
    getTemperaturesAndFanSpeed();
    getCurrentState();

}


// Constructor
AldesDriver::AldesDriver()
{
     _mb_master = new ModbusMaster((mb_comm_mode_t)CONFIG_MB_COMM_MODE, 
                                (uart_port_t)CONFIG_MB_UART_PORT_NUM,
                                CONFIG_MB_UART_TXD,
                                CONFIG_MB_UART_RXD,
                                CONFIG_MB_UART_RTS,
                                CONFIG_MB_UART_CTS,
                                CONFIG_MB_UART_BAUD_RATE,
                                (uart_word_length_t)CONFIG_MB_UART_DATA_BITS,
                                (uart_parity_t)CONFIG_MB_UART_PARITY,
                                (uart_stop_bits_t)CONFIG_MB_UART_STOP_BITS,
                                1000,
                                (uart_mode_t)CONFIG_MB_UART_MODE);
}

void AldesDriver::start(uint64_t delay_ms)
{

    getDeviceInfos(); // TODO set a retry task if not succeed
    getSpeedSettings();
    getRegulationParams();
    getFanConfig();

    getFilterTimerState();  // TODO put in another daily periodicTask
    getSeasonDetection();  // TODO put in another daily periodicTask
    getDate();
    getErrorCode(); // TODO When do we want to check this

    getTBypassSummer();// TODO When do we want to check this Set as well


    setUserLevel(3);
    _periodicTask = new PeriodicSoftTask(&AldesDriver::trigger_reading, 
                            this, delay_ms, "aldesT");
    
}

void AldesDriver::stop(void)
{
    delete _periodicTask;
    _periodicTask = nullptr;
}

void AldesDriver::getDeviceInfos()
{
    mb_data gen_data = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_PRODUCT_CODE,
                                            6);
                                        
    if (gen_data.getSize() > 0)
        _data.product_code = gen_data;

    ESP_LOGD(ALDES_TAG, "Product Code : %d - %s", 
                    _data.product_code, aldesDeviceFromCode(_data.product_code));

    _data.serial_num = gen_data.getDataFrom(4);

    ESP_LOGD(ALDES_TAG, "Serial Number : %d ", _data.serial_num);

    mb_data firm_ver = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_SOFT_VERSION,
                                            1);
    if (firm_ver.getSize() > 0)
        _data.firm_ver = firm_ver;

    ESP_LOGD(ALDES_TAG, "Firmware version : %d", _data.firm_ver);
    
}

void AldesDriver::getSpeedSettings()
{
    mb_data speed_settings = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_SETTING_MVE_VACATION,
                                            10);
                                        
    if (speed_settings.getSize() > 0)
    {
        _data.setting_MVE_vacation   = speed_settings;
        _data.setting_MVI_vacation   = speed_settings.getDataFrom(2);
        _data.setting_MVE_daily      = speed_settings.getDataFrom(4);
        _data.setting_MVI_daily      = speed_settings.getDataFrom(6);
        _data.setting_MVE_pushButton = speed_settings.getDataFrom(8);
        _data.setting_MVI_pushButton = speed_settings.getDataFrom(10);
        _data.setting_MVE_boost      = speed_settings.getDataFrom(12);
        _data.setting_MVI_boost      = speed_settings.getDataFrom(14);
        _data.setting_MVE_maxSpeed   = speed_settings.getDataFrom(16);
        _data.setting_MVI_maxSpeed   = speed_settings.getDataFrom(18);
    }

    ESP_LOGV(ALDES_TAG, "--------------------------------------------------------------");
    ESP_LOGV(ALDES_TAG, "|   | Vacation |  Daily   | PushButton |  Boost   | MaxSpeed |");
    ESP_LOGV(ALDES_TAG, "|---|----------|----------|------------|----------|----------|");
    ESP_LOGV(ALDES_TAG, "|MVE|    %d    |    %d   |    %d     |    %d   |    %d   |",
            _data.setting_MVE_vacation, _data.setting_MVE_daily, _data.setting_MVE_pushButton, 
            _data.setting_MVE_boost,_data.setting_MVE_maxSpeed);
    ESP_LOGV(ALDES_TAG, "|MVI|    %d    |    %d   |    %d     |    %d   |    %d   |",
            _data.setting_MVI_vacation, _data.setting_MVI_daily, _data.setting_MVI_pushButton, 
            _data.setting_MVI_boost,_data.setting_MVI_maxSpeed);
    ESP_LOGV(ALDES_TAG, "--------------------------------------------------------------"); 

}

void AldesDriver::getRegulationParams()
{
    mb_data regul_params = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_REGULATION_MODE,
                                            4);
                                        
    if (regul_params.getSize() > 0)
    {
        _data.regulation_mode    = regul_params;
        _data.demand_user        = regul_params.getDataFrom(2);
        _data.demand_programmer  = regul_params.getDataFrom(4);
        _data.bypass_mode        = regul_params.getDataFrom(6);
    }

    regul_params = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_TEMPO_FILTER,
                                            1);
    
    if (regul_params.getSize() > 0)
        _data.tempo_filter = regul_params;
    
    regul_params = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_UNBALANCED_COEF_MVI,
                                            1);
    ESP_LOGV(ALDES_TAG, "----------------------");
    ESP_LOGV(ALDES_TAG, "| Regul Mode  |  %d   |", _data.regulation_mode );
    ESP_LOGV(ALDES_TAG, "| Demand User |  %d   |", _data.demand_user );
    ESP_LOGV(ALDES_TAG, "| Demand Prog |  %d   |", _data.demand_programmer );
    ESP_LOGV(ALDES_TAG, "| Bypass Mode |  %d   |", _data.bypass_mode );
    ESP_LOGV(ALDES_TAG, "| Tempo Filter|  %d  |", _data.tempo_filter );
    if (regul_params.getSize() > 0)
        ESP_LOGV(ALDES_TAG, "| Unbalanced C|  %d |", (int16_t)regul_params);
    ESP_LOGV(ALDES_TAG, "----------------------");

}

void AldesDriver::getFanConfig()
{
    mb_data fan_cfg = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_FAN_CONFIG,
                                            1);
    if (fan_cfg.getSize() > 0)
    {
        _data.fan_config = fan_cfg;
    }
    
    ESP_LOGV(ALDES_TAG, "-------------Season----------");
    ESP_LOGV(ALDES_TAG, "|2:Config A/1:Config B/0:None|");
    ESP_LOGV(ALDES_TAG, "|        %d                  |", 
                    _data.fan_config);
    ESP_LOGV(ALDES_TAG, "------------------------------");
}

void AldesDriver::getFilterTimerState()
{
    mb_data filterState = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_FILTER_STATE_PERCENT,
                                            2);
    if (filterState.getSize() > 0)
    {
        _data.filter_state_percent = filterState;
        _data.filter_state_days = filterState.getDataFrom(2);
    }
    
    ESP_LOGV(ALDES_TAG, "---Filter state---");
    ESP_LOGV(ALDES_TAG, "|   %  |   days  |");
    ESP_LOGV(ALDES_TAG, "|  %d |  %d |", 
                _data.filter_state_percent, _data.filter_state_days);
    ESP_LOGV(ALDES_TAG, "------------------");
}

void AldesDriver::getSeasonDetection()
{
     mb_data season = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_SEASON_DETECTION,
                                            1);
    if (season.getSize() > 0)
    {
        _data.season_detection = season;

    }
    
    ESP_LOGV(ALDES_TAG, "-------------Season----------");
    ESP_LOGV(ALDES_TAG, "|0:Unknown/1:Winter/2:Summer|");
    ESP_LOGV(ALDES_TAG, "|        %d                  |", 
                    _data.season_detection);
    ESP_LOGV(ALDES_TAG, "------------------------------");
}

void AldesDriver::getDate()  // TODO Set Date and check how datetime32 is formatted
{
    mb_data date = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_DATETIME32,
                                            2);
    
    if (date.getSize() > 0)
        _data.date_time = date;

    
    date = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_DATE_YEAR,
                                            7);

    if (date.getSize() > 0)
    {
        ESP_LOGV(ALDES_TAG, "------Date Time---------");
        ESP_LOGV(ALDES_TAG, "| datetime32:  %d  |", _data.date_time );
        ESP_LOGV(ALDES_TAG, "| %d-%d-%d day: %d   |",
                    (uint16_t)date,
                    (uint16_t)date.getDataFrom(2),
                    (uint16_t)date.getDataFrom(4),
                    (uint16_t)date.getDataFrom(6)
                );
        ESP_LOGV(ALDES_TAG, "| %d:%d:%d       |",
                    (uint16_t)date.getDataFrom(8),
                    (uint16_t)date.getDataFrom(10),
                    (uint16_t)date.getDataFrom(12)
                );
        ESP_LOGV(ALDES_TAG, "-------------------------");
    }
    
}

void AldesDriver::getErrorCode()
{
    mb_data error = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_ERROR_CODE,
                                            1);
    if (error.getSize() > 0)
    {
        _data.error_code = error;

    }
    
    ESP_LOGV(ALDES_TAG, "-------------Error----------");
    ESP_LOGV(ALDES_TAG, "|        %d                  |", 
                    _data.error_code);
    ESP_LOGV(ALDES_TAG, "------------------------------");

}

void AldesDriver::getTBypassSummer()
{
    mb_data t_bypass = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_T_BYPASS_SUMMER,
                                            1);
    if (t_bypass.getSize() > 0)
    {
        _data.T_bypass_summer = t_bypass;

    }
    
    ESP_LOGV(ALDES_TAG, "-------T° bypass summer-------");
    ESP_LOGV(ALDES_TAG, "|        %d                |", 
                    _data.T_bypass_summer);
    ESP_LOGV(ALDES_TAG, "------------------------------");

}

void AldesDriver::getBypassPosition()
{
     mb_data bypassPos = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_BYPASS_POSITION,
                                            1);
    if (bypassPos.getSize() > 0)
        _data.bypass_position    = bypassPos;
    
    ESP_LOGV(ALDES_TAG, "Bypass position : %d", 
                _data.bypass_position);
}

void AldesDriver::getTemperaturesAndFanSpeed()
{
    mb_data data = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                    AldesModbus::REG_T_INTAKE_AIR_OUT,
                                    2);
    if (data.getSize() > 0){
        _data.T_intake_air_out = data;
        _data.T_extract_air_in = data.getDataFrom(2);
    }
    
    // Accessing level 3 register
    data = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                    AldesModbus::REG_T_SUPPLY_AIR_IN,
                                    7);
    if (data.getSize() > 0)
    {
        if ((int16_t)data == (int16_t)0xffff){ // User Level is not set correctly
            ESP_LOGI(ALDES_TAG, "Set user level to 3");
            setUserLevel(3);
        } else {
            _data.T_supply_air_in    = data;
            _data.T_exhaust_air_out  = data.getDataFrom(2);
            _data.speed_exhaust_fan  = data.getDataFrom(4);
            _data.speed_supply_fan   = data.getDataFrom(6);
            _data.airflow_MVE        = data.getDataFrom(8);
            _data.airflow_MVI        = data.getDataFrom(10);
            _data.pressure           = data.getDataFrom(12);
        }

    }

    ESP_LOGV(ALDES_TAG, "-----------------------------------------------------------------------------");
    ESP_LOGV(ALDES_TAG, "| T_intake_air_out | T_extract_air_in | T_supply_air_in | T_exhaust_air_out |");
    ESP_LOGV(ALDES_TAG, "|        %d      |        %d      |        %d     |         %d      |",
                        _data.T_intake_air_out,   _data.T_extract_air_in, _data.T_supply_air_in, _data.T_exhaust_air_out);
    ESP_LOGV(ALDES_TAG, "|------------------|------------------|-----------------|-------------------|");
    ESP_LOGV(ALDES_TAG, "|Speed_Exhaust_Fan | Speed_Supply_Fan |   Airflow_MVE   |    Airflow_MVI    |");
    ESP_LOGV(ALDES_TAG, "|     %d rpm     |    %d rpm      |     %d m3/h    |       %d m3/h    |",
                        _data.speed_exhaust_fan,   _data.speed_supply_fan, _data.airflow_MVE, _data.airflow_MVI);
    ESP_LOGV(ALDES_TAG, "-----------------------------------------------------------------------------");
    ESP_LOGV(ALDES_TAG, "Pressure : %d",    _data.pressure);     

}

void AldesDriver::getCurrentState()
{
    mb_data state = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_CURRENT_LEVEL,
                                            2);
    if (state.getSize() > 0)
    {
        _data.current_level = state;
        _data.requester = state.getDataFrom(2);
    }
    
    ESP_LOGV(ALDES_TAG, "------Current State-----");
    ESP_LOGV(ALDES_TAG, "| current lvl:  %d  |",_data.current_level);
    ESP_LOGV(ALDES_TAG, "| requester:   %d   |",_data.requester);
    ESP_LOGV(ALDES_TAG, "------------------------");
}

void AldesDriver::setUserLevel(uint8_t lvl)
{
    mb_data usr_lvl(2);

    switch (lvl){
        case 1:
            usr_lvl = (uint16_t)AldesModbus::USR_LVL_1;
            break;
        case 2:
            usr_lvl = (uint16_t)AldesModbus::USR_LVL_2;
            break;
        case 3:
            usr_lvl = (uint16_t)AldesModbus::USR_LVL_3;
            break;
        default:
            usr_lvl = (uint16_t)AldesModbus::USR_LVL_NORMAL;
            break;
    }
    
    _mb_master->writeRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_USER_LEVEL,
                                            usr_lvl);
    if (usr_lvl.getSize() > 0)
        ESP_LOGD(ALDES_TAG, "user level : %d", (uint16_t)usr_lvl);
}



const char* AldesDriver::aldesDeviceFromCode(uint32_t code)
{
    size_t i;

    for (i = 0; i < sizeof(AldesModbus::aldesDevice_table) / sizeof(AldesModbus::aldesDevice_table[0]); ++i) {
        if (AldesModbus::aldesDevice_table[i].code == code) {
            return AldesModbus::aldesDevice_table[i].msg;
        }
    }
    return AldesModbus::unknown_device;
}



/// Events ////////////////////////////////////////////////////////////////////////////
void AldesDriver::postEvent(uint8_t channel, double value)
{

    if(_adsCallbacks.contains(channel)) {
        ESP_LOGV(ALDES_TAG, "Callback channel %d is found - value: %f", channel, value);
        ZbNode::_eventLoop->enqueue(std::bind(std::ref(_adsCallbacks.at(channel)), value));
    } else {
        ESP_LOGD(ALDES_TAG, "No Callback registered for channel %d", channel);
    }
        
}