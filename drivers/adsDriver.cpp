/*
  zTank 
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <esp_log.h> // TODEL development purpose

#include "adsDriver.h"
#include "zbNode.h"

static const char *ADS_TAG = "AdsDriver";


// Static event handler
void AdsDriver::ads1115_event_handler(uint16_t input, double value)
{
    ESP_LOGV(ADS_TAG, "Callback Main Ads1115 input: %d - value: %f", input-4, value);

    // Post event to update the attribute of concerning registered clusters
    AdsDriver::getInstance().postEvent(input-4, value);

}

// Event handler for periodic soft task
void AdsDriver::trigger_conversion()
{
    vTaskDelay(_tickToWait);
    _ads.getVoltage(Ads1115::MUX_0_GND);
    vTaskDelay(_tickToWait);
    _ads.getVoltage(Ads1115::MUX_2_GND);
    vTaskDelay(_tickToWait);
    _ads.getVoltage(Ads1115::MUX_3_GND);
    vTaskDelay(_tickToWait);
    _ads.getVoltage(Ads1115::MUX_1_GND);
}

AdsDriver& AdsDriver::getInstance() 
{
    static AdsDriver instance; // Guaranteed to be destroyed.
                                // Instantiated on first use.
    return instance;
}

// Private constructor
AdsDriver::AdsDriver(): 
        _i2c_master(I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, true),
        _ads(&_i2c_master,Ads1115::Addr_Gnd, ADS_I2C_FREQ_HZ)
{

}

void AdsDriver::start(uint64_t delay_ms)
{
    _periodicTask = new PeriodicSoftTask(&AdsDriver::trigger_conversion, 
                            this, delay_ms, "ads1115");
    
}

void AdsDriver::stop(void)
{
    delete _periodicTask;
    _periodicTask = nullptr;
}

void AdsDriver::setup(void)
{
    ESP_LOGD(ADS_TAG, "Configuration ADS1115 Start --------------");

    Ads1115::Cfg_reg cfg = _ads.getConfig();
    ESP_LOGD(ADS_TAG, "Config: %d", cfg.reg.reg);
    ESP_LOGD(ADS_TAG, "COMP QUE:    %x",cfg.bit.COMP_QUE);
    ESP_LOGD(ADS_TAG, "COMP LAT:    %x",cfg.bit.COMP_LAT);
    ESP_LOGD(ADS_TAG, "COMP POL:    %x",cfg.bit.COMP_POL);
    ESP_LOGD(ADS_TAG, "COMP MODE:   %x",cfg.bit.COMP_MODE);
    ESP_LOGD(ADS_TAG, "DataRate:    %x",cfg.bit.DR);
    ESP_LOGD(ADS_TAG, "MODE:        %x",cfg.bit.MODE);
    ESP_LOGD(ADS_TAG, "PGA:         %x",cfg.bit.PGA);
    ESP_LOGD(ADS_TAG, "MUX:         %x",cfg.bit.MUX);
    ESP_LOGD(ADS_TAG, "OS:          %x",cfg.bit.OS);

    Ads1115::reg2Bytes_t regData;
    _ads.setPga(Ads1115::FSR_4_096); // Setting range for PGA optimized to 3.3V Power supply
    _ads.setSps(Ads1115::SPS_64);   // Setting data rate

    // event handler shall have signature void(uint16_t input, int16_t value)
    _ads.setReadyPin(GPIO_NUM_3, &ads1115_event_handler);

    regData = _ads.readRegister(Ads1115::reg_configuration);
    ESP_LOGD(ADS_TAG, "Configuration : %x", regData.reg);

    regData = _ads.readRegister(Ads1115::reg_lo_thresh);
    ESP_LOGD(ADS_TAG, "Reg Lo Thresh : %x", regData.reg);

    regData = _ads.readRegister(Ads1115::reg_hi_thresh);
    ESP_LOGD(ADS_TAG, "Reg Hi Thresh : %x", regData.reg);

    // Should be at the end of setup to be sure that config is correct
    _tickToWait = _ads.tickToWait();
    ESP_LOGD(ADS_TAG,"Conversion duration in tick: %d",_tickToWait);
    
}
/*
void AdsDriver::setVoltage(uint8_t channel, double value)
{
    if ( channel>3)
    {
        ESP_LOGE(ADS_TAG, "Unable to write  channel > 3 (was %d)",  channel);
        return;
    }

    _voltage[ channel] = value;
}

double AdsDriver::getVoltage(uint8_t  channel)
{
    if ( channel>3)
    {
        ESP_LOGE(ADS_TAG, "Unable to read  channel > 3 (was %d)",  channel);
        return 0.0f;
    }
    return _voltage[channel];
}
*/

/// Events ////////////////////////////////////////////////////////////////////////////
void AdsDriver::postEvent(uint8_t channel, double value)
{

    if(_adsCallbacks.contains(channel)) {
        ESP_LOGV(ADS_TAG, "Callback channel %d is found - value: %f", channel, value);
        ZbNode::_eventLoop->enqueue(std::bind(std::ref(_adsCallbacks.at(channel)), value));
    } else {
        ESP_LOGD(ADS_TAG, "No Callback registered for channel %d", channel);
    }
        
}
