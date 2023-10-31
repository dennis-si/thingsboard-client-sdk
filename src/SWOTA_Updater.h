#ifndef SWOTA_Updater_h
#define SWOTA_Updater_h

// Local include.
#include "Configuration.h"

#if THINGSBOARD_ENABLE_SWOTA

// Library include.
#include <stddef.h>
#include <stdint.h>

#include <esp_spiffs.h>



/// @brief IUpdater implementation that uses the Over the Air Update API from Espressif (https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html)
/// under the hood to write the given binary firmware data into flash memory so we can restart with newly received firmware
class SWOTA_Updater {
  public:
    SWOTA_Updater();

    bool begin(const size_t& software_size);
  
    size_t write(uint8_t* payload, const size_t& total_bytes);

    void reset();
  
    bool end();

    private:
      uint32_t m_ota_handle;
      const void *m_update_partition;
      esp_vfs_spiffs_conf_t* confPtr;
      FILE* swFile;
};



#endif // THINGSBOARD_ENABLE_SWOTA

#endif // SWOTA_Updater_h