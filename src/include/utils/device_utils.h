#pragma once
#include <Arduino.h>
#include "../mqtt_module.h"
/*
    Contain all the device utils
*/ 

namespace DeviceUtils {
    String getMacAddress();
}