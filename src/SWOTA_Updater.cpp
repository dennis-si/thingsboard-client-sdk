// Header include.
#include "SWOTA_Updater.h"
#include "esp_log.h"

#if THINGSBOARD_ENABLE_SWOTA



// Library include.



SWOTA_Updater::SWOTA_Updater() :
    m_ota_handle(0U),
    m_update_partition(nullptr)
{
    // Nothing to do
}

bool SWOTA_Updater::begin(const size_t& software_size) {

    // esp_vfs_spiffs_conf_t conf = {
    //   .base_path = "/spiffs",
    //   .partition_label = NULL,
    //   .max_files = 5,
    //   .format_if_mount_failed = true
    // };

    // confPtr = &conf;

    // // Use settings defined above to initialize and mount SPIFFS filesystem.
    // // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    // esp_err_t err = esp_vfs_spiffs_register(&conf);
    // ESP_LOGI("SWOTA", "Error %d", err);
    // if (err != ESP_OK) return false;


    swFile = fopen("/spiffs/Diverter.enc", "w");
    if (swFile == NULL) {
        return false;
    }
    return true;
}

size_t SWOTA_Updater::write(uint8_t* payload, const size_t& total_bytes) {
    
    const size_t written_bytes = fwrite(payload, 1, total_bytes, swFile);
    
    if (written_bytes != total_bytes) return 0U;

    return written_bytes;
}

void SWOTA_Updater::reset() {
    if (swFile != NULL) fclose(swFile);

    // All done, unmount partition and disable SPIFFS
    if (confPtr != NULL) esp_vfs_spiffs_unregister(confPtr->partition_label);
}

bool SWOTA_Updater::end() {

    if (swFile != NULL) fclose(swFile);

    // All done, unmount partition and disable SPIFFS
    //esp_err_t error = esp_vfs_spiffs_unregister(confPtr->partition_label);

    return true;
}



#endif // THINGSBOARD_ENABLE_SWOTA
