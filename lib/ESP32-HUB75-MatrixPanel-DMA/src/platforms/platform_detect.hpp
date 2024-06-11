/*----------------------------------------------------------------------------/
Original Source:
 https://github.com/lovyan03/LovyanGFX/

Licence:
 [FreeBSD](https://github.com/lovyan03/LovyanGFX/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)

Contributors:
 [ciniml](https://github.com/ciniml)
 [mongonta0716](https://github.com/mongonta0716)
 [tobozo](https://github.com/tobozo)

Modified heavily for the ESP32 HUB75 DMA library by:
 [mrfaptastic](https://github.com/mrfaptastic)
/----------------------------------------------------------------------------*/
#pragma once

#if defined (ESP_PLATFORM)

 #include <sdkconfig.h>

 #if defined (CONFIG_IDF_TARGET_ESP32S3)
  #pragma message "HUB75: Compiling for ESP32-S3"
  #include "esp32s3/gdma_lcd_parallel16.hpp"
  #include "esp32s3/esp32s3-default-pins.hpp"

 #elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32H2)
  #error "HUB75: Unsupported platform, ESP32 RISC-V devices do not have an LCD interface and are therefore not supported by this library."

 #else
  #error "HUB75: Unsupported platform, this version of the library is specific to the ESP32-S3."

 #endif

#else
 #error "HUB75: Unknown platform."

#endif

