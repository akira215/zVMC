/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "zbAnalogValueCluster.h"
#include "aldesDriver.h"

/// @brief Store current airflow
/// @brief AnalogValueCluster
/// @param PresentValue airflow in m3/h
class AirflowCluster: public ZbAnalogValueCluster
{

public:
    AirflowCluster():ZbAnalogValueCluster()  { };

    // Called when Modbus is sending data
    void setAirflow(int16_t flowrate) {
        float_t flow = static_cast<float_t>(flowrate);
        ESP_LOGV(ZCLUSTER_TAG, "set airflow : %f", 
                        flow);
        // will be read as uint16_t
      	setAttribute(ESP_ZB_ZCL_ATTR_ANALOG_VALUE_PRESENT_VALUE_ID,
                &flow);
    }

}; // AirflowCluster
