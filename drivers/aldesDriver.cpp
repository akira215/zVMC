/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h> // TODEL development purpose

#include "aldesDriver.h"
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
    _mb_master->testRequest();
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
    _periodicTask = new PeriodicSoftTask(&AldesDriver::trigger_reading, 
                            this, delay_ms, "aldes");
    
}

void AldesDriver::stop(void)
{
    delete _periodicTask;
    _periodicTask = nullptr;
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