/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbAnalogValueCluster.h"
#include "aldesDriver.h"

// Store remaining days prior to a filter change is required
class AirflowMVECluster: public ZbAnalogValueCluster
{

public:
    AirflowMVECluster():ZbAnalogValueCluster()  { };

    // Called when Modbus is sending data
    void setMVEAirflow(int16_t flowrate) {
        float_t flow = static_cast<float_t>(flowrate);
        ESP_LOGW(ZCLUSTER_TAG, "setMVEAirflow : %f", 
                        flow);
        // will be read as uint16_t
      	setAttribute(ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID,
                &flow);
    }

}; // FilterTimerCluster
