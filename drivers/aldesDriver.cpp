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
void AldesDriver::ads1115_event_handler(uint16_t input, double value)
{
    ESP_LOGV(ALDES_TAG, "Callback Main Ads1115 input: %d - value: %f", input-4, value);

    // Post event to update the attribute of concerning registered clusters
    //AldesDriver::getInstance().postEvent(input-4, value);

}

// Event handler for periodic soft task
void AldesDriver::trigger_reading()
{
    mb_data temp = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_BYPASS_POSITION,
                                            1);
    if (temp.getSize() > 0)
        ESP_LOGI(ALDES_TAG, "Reg 0x%04x Slave answer : %d", 
                AldesModbus::REG_BYPASS_POSITION, (uint16_t)temp);
    
    temp = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                    AldesModbus::REG_T_INTAKE_AIR_OUT,
                                    2);
    if (temp.getSize() > 0)
        ESP_LOGI(ALDES_TAG, "Intake air outside T° : %d  - Extracted air inside T°: %d", 
                AldesModbus::REG_T_INTAKE_AIR_OUT , (int16_t)temp, (int16_t)temp.getDataFrom(2));
    
    
    ////// ESP_LOGW(ALDES_TAG, "Reading register level 3");
    temp = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                    AldesModbus::REG_T_SUPPLY_AIR_IN,
                                    7);
    if (temp.getSize() > 0)
    {
        if ((int16_t)temp == (int16_t)0xffff){ // User Level is not set correctly
            ESP_LOGI(ALDES_TAG, "Set user level to 3");
            setUserLevel(3);
        } else {
            ESP_LOGI(ALDES_TAG, "Supply air inside T°: %d - Exhaust air outside %d", 
                        (int16_t)temp, (int16_t)temp.getDataFrom(2));
            
            ESP_LOGI(ALDES_TAG, "Exhaust Fan: %d rpm - Supply Fan %d rpm", 
                        (int16_t)temp.getDataFrom(4), (int16_t)temp.getDataFrom(6));
            
            ESP_LOGI(ALDES_TAG, "Exhaust Airflow: %d m3/h - Supply Airflow %d m3/h", 
                        (int16_t)temp.getDataFrom(8), (int16_t)temp.getDataFrom(10));
            
            ESP_LOGI(ALDES_TAG, "Pressure: %d Pa (0.1)", 
                        (int16_t)temp.getDataFrom(12));
        }

        
    }
    
    
    //_mb_master->testRequest();

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

    getDeviceInfos();
    getSpeedSettings();
    setUserLevel(3);
    _periodicTask = new PeriodicSoftTask(&AldesDriver::trigger_reading, 
                            this, delay_ms, "aldes");
    
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
        _product_code = gen_data;

    ESP_LOGD(ALDES_TAG, "Product Code : %d - %s", 
                    _product_code, aldesDeviceFromCode(_product_code));

    _serial_num = gen_data.getDataFrom(4);

    ESP_LOGD(ALDES_TAG, "Serial Number : %d ", _serial_num);

    mb_data firm_ver = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_SOFT_VERSION,
                                            1);
    if (firm_ver.getSize() > 0)
        _firm_ver = firm_ver;

    ESP_LOGD(ALDES_TAG, "Firmware version : %d", _firm_ver);
    
}

void AldesDriver::getSpeedSettings()
{
    mb_data speed_settings = _mb_master->readRegisters(AldesModbus::MB_ALDES_ADDR,
                                            AldesModbus::REG_SETTING_MVE_VACATION,
                                            10);
                                        
    if (speed_settings.getSize() > 0)
    {
        ESP_LOGI(ALDES_TAG, "Vacation MVE: %d m3/h- MVI: %d m3/h", 
                (uint16_t)speed_settings, 
                (uint16_t)speed_settings.getDataFrom(2));
        ESP_LOGI(ALDES_TAG, "Daily MVE: %d m3/h- MVI: %d m3/h", 
                (uint16_t)speed_settings.getDataFrom(4), 
                (uint16_t)speed_settings.getDataFrom(6));
        ESP_LOGI(ALDES_TAG, "Push Button MVE: %d m3/h- MVI: %d m3/h",  
                (uint16_t)speed_settings.getDataFrom(8), 
                (uint16_t)speed_settings.getDataFrom(10));
        ESP_LOGI(ALDES_TAG, "Boost MVE: %d m3/h- MVI: %d m3/h",  
                (uint16_t)speed_settings.getDataFrom(12), 
                (uint16_t)speed_settings.getDataFrom(14));
        ESP_LOGI(ALDES_TAG, "Max Speed MVE: %d m3/h- MVI: %d m3/h", 
                (uint16_t)speed_settings.getDataFrom(16), 
                (uint16_t)speed_settings.getDataFrom(18));
    }
        

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