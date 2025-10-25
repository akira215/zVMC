/*
  zVMC
  Repository: https://github.com/akira215/
  License: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  Author: Akira Shimahara
*/

#pragma once

#include "ZbTemperatureMeasCluster.h"

class TSensorCluster: public ZbTemperatureMeasCluster
{

public:
    TSensorCluster() {};
    void setTemperatureMeasuredValue(int16_t newValue) {
      setAttribute(ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                &newValue);
      }

}; // TsensorCluster