#ifndef SWOTA_Update_Callback_h
#define SWOTA_Update_Callback_h

// Local includes.
#include "Callback.h"




#if THINGSBOARD_ENABLE_SWOTA

// Local includes.
#include "SWOTA_Updater.h"



/// ---------------------------------
/// Constant strings in flash memory.
/// ---------------------------------
#if THINGSBOARD_ENABLE_PROGMEM
constexpr char OTA_CB_IS_NULL[] PROGMEM = "OTA update callback is NULL";
#else
constexpr char SW_OTA_CB_IS_NULL[] = "OTA update callback is NULL";
#endif // THINGSBOARD_ENABLE_PROGMEM

// OTA default values.
#if THINGSBOARD_ENABLE_PROGMEM
constexpr uint8_t CHUNK_RETRIES PROGMEM = 12U;
constexpr uint16_t CHUNK_SIZE PROGMEM = (4U * 1024U);
constexpr uint64_t REQUEST_TIMEOUT PROGMEM = (5U * 1000U * 1000U);
#else
constexpr uint8_t SW_CHUNK_RETRIES = 12U;
constexpr uint16_t SW_CHUNK_SIZE = (4U * 1024U);
constexpr uint64_t SW_REQUEST_TIMEOUT = (5U * 1000U * 1000U);
#endif // THINGSBOARD_ENABLE_PROGMEM


/// @brief Over the air software update callback wrapper,
/// contains the needed configuration settings to create the request that should be sent to the server.
/// Documentation about the specific use of Over the air updates in ThingsBoard can be found here https://thingsboard.io/docs/user-guide/ota-updates/
class SWOTA_Update_Callback : public Callback<void, const bool&> {
  public:
    /// @brief OTA software update callback signature
    using returnType = void;
    using progressArgumentType = const size_t&;
    using progressFn = std::function<returnType(progressArgumentType current, progressArgumentType total)>;

    /// @brief Constructs empty callback, will result in never being called
    SWOTA_Update_Callback();

    /// @brief Constructs callbacks that will be called when the OTA software data,
    /// has been completly sent by the cloud, received by the client and written to the flash partition
    /// @param endCb End callback method that will be called as soon as the OTA software update, either finished successfully or failed
    /// @param currSwTitle Software title the device has choosen, is used to only allow updates with the same given title, other updates will be canceled
    /// @param currSwVersion Software version the device is currently on, is usded to only allow updates with a different version, other updates will be canceled
    /// @param updater Updater implementation that writes the given software data
    /// @param chunkRetries Amount of retries the OTA software update has to download each seperate chunk with a given size,
    /// before the complete download is stopped and registered as failed
    /// @param chunkSize Size of the chunks that the software binary data will be split into,
    /// increased chunkSize might speed up the process by a little bit, but requires more heap memory,
    // because the whole chunk is saved into the heap before it can be processed and is then erased again after it has been used
    /// @param timeout Maximum amount of time in microseconds for the OTA software update for each seperate chunk,
    /// until that chunk counts as a timeout, retries is then subtraced by one and the download is retried
    SWOTA_Update_Callback(function endCb, const char *currSwTitle, const char *currSwVersion, SWOTA_Updater *updater, const uint8_t &chunkRetries = SW_CHUNK_RETRIES, const uint16_t &chunkSize = SW_CHUNK_SIZE, const uint64_t &timeout = SW_REQUEST_TIMEOUT);

    /// @brief Constructs callbacks that will be called when the OTA software data,
    /// has been completly sent by the cloud, received by the client and written to the flash partition as well as callback
    // that will be called every time the current progress of the software update changes
    /// @param progressCb Progress callback method that will be called every time our current progress of downloading the complete software data changed,
    /// this makes it possible to display a progress bar or signal easily how far we are in the current downloading process
    /// @param endCb End callback method that will be called as soon as the OTA software update, either finished successfully or failed
    /// @param currSwTitle Software title the device has choosen, is used to only allow updates with the same given title, other updates will be canceled
    /// @param currSwVersion Software version the device is currently on, is usded to only allow updates with a different version, other updates will be canceled
    /// @param updater Updater implementation that writes the given software data
    /// @param chunkRetries Amount of retries the OTA software update has to download each seperate chunk with a given size,
    /// before the complete download is stopped and registered as failed
    /// @param chunkSize Size of the chunks that the software binary data will be split into,
    /// increased chunkSize might speed up the process by a little bit, but requires more heap memory,
    // because the whole chunk is saved into the heap before it can be processed and is then erased again after it has been used
    /// @param timeout Maximum amount of time in microseconds for the OTA software update for each seperate chunk,
    /// until that chunk counts as a timeout, retries is then subtraced by one and the download is retried
    SWOTA_Update_Callback(progressFn progressCb, function endCb, const char *currSwTitle, const char *currSwVersion, SWOTA_Updater *updater, const uint8_t &chunkRetries = SW_CHUNK_RETRIES, const uint16_t &chunkSize = SW_CHUNK_SIZE, const uint64_t &timeout = SW_REQUEST_TIMEOUT);

    /// @brief Calls the progress callback that was subscribed, when this class instance was initally created
    /// @tparam Logger Logging class that should be used to print messages generated by internal processes
    /// @param current Already received and processs amount of chunks
    /// @param total Total amount of chunks we need to receive and process until the update has completed
    template<typename Logger>
    inline returnType Call_Progress_Callback(progressArgumentType current, progressArgumentType total) const {
        // Check if the callback is a nullptr,
        // meaning it has not been assigned any valid callback method
        if (!m_progressCb) {
          Logger::log(SW_OTA_CB_IS_NULL);
          return returnType();
        }
        return m_progressCb(current, total);
    }

    /// @brief Sets the progress callback method that will be called every time our current progress of downloading the complete software data changed,
    /// this makes it possible to display a progress bar or signal easily how far we are in the current downloading process
    /// @param progressCb Progress callback method that will be called
    void Set_Progress_Callback(progressFn progressCb);

    /// @brief Gets the current software title, used to decide if an OTA software update is already installed and therefore should not be downladed,
    /// this is only done if the title of the update and the current software title are the same because if they are not then this software is meant for another device type
    /// @return Current software title of the device
    const char* Get_Software_Title() const;

    /// @brief Sets the current software title, used to decide if an OTA software update is already installed and therefore should not be downladed,
    /// this is only done if the title of the update and the current software title are the same because if they are not then this software is meant for another device type
    /// @param currSwTitle Current software title of the device
    void Set_Software_Title(const char *currSwTitle);

    /// @brief Gets the current software version, used to decide if an OTA software update is already installed and therefore should not be downladed,
    /// this is only done if the version of the update and the current software version are different, because if they are not then we would download the same software as is already on the device
    /// @return Current software version of the device
    const char* Get_Software_Version() const;

    /// @brief Sets the current software version, used to decide if an OTA software update is already installed and therefore should not be downladed,
    /// this is only done if the version of the update and the current software version are different, because if they are not then we would download the same software as is already on the device
    /// @param currSwVersion Current software version of the device
    void Set_Software_Version(const char *currSwVersion);

    /// @brief Gets the updater implementation, used to write the actual software data into the needed memory location,
    /// so it can be used to reboot the given device with that new flashed software
    /// @return Updater implementation that writes the given software data
    SWOTA_Updater* Get_Updater() const;

    /// @brief Sets the updater implementation, used to write the actual software data into the needed memory location,
    /// so it can be used to reboot the given device with that new flashed software
    /// @param updater Updater implementation that writes the given software data
    void Set_Updater(SWOTA_Updater *updater);

    /// @brief Gets the amount of times we attempt to download each chunk of the OTA software binary file,
    /// if the download fails because it times out, doesn't let itself write into flash memory, ...
    /// the retries are decreased by 1 until we hit 0, if that is the case then we instead stop the OTA software update completely
    /// @return Amount of retries for each single chunk before we abort the update
    const uint8_t& Get_Chunk_Retries() const;

    /// @brief Sets the amount of times we attempt to download each chunk of the OTA software binary file,
    /// if the download fails because it times out, doesn't let itself write into flash memory, ...
    /// the retries are decreased by 1 until we hit 0, if that is the case then we instead stop the OTA software update completely
    /// @param chunkRetries Amount of retries for each single chunk before we abort the update
    void Set_Chunk_Retries(const uint8_t &chunkRetries);

    /// @brief Gets the size of the chunks that the software binary data will be split into,
    /// increased chunkSize might speed up the process by a little bit, but requires more heap memory,
    // because the whole chunk is saved into the heap before it can be processed and is then erased again after it has been used
    /// @return Size of each single chunk to be downloaded
    const uint16_t& Get_Chunk_Size() const;

    /// @brief Sets the size of the chunks that the software binary data will be split into,
    /// increased chunkSize might speed up the process by a little bit, but requires more heap memory,
    // because the whole chunk is saved into the heap before it can be processed and is then erased again after it has been used
    /// @param chunkSize Size of each single chunk to be downloaded
    void Set_Chunk_Size(const uint16_t &chunkSize);

    /// @brief Gets the time in microseconds we wait until we declare a single chunk we attempted to download as a failure
    /// @return Timeout time until we expect a response from the server
    const uint64_t& Get_Timeout() const;

    /// @brief Sets the time in microseconds we wait until we decleare a single chunk we attempted to download as a timeout
    /// @param timeout_microseconds Timeout time until we expect a response from the server
    void Set_Timeout(const uint64_t &timeout_microseconds);

  private:
    progressFn      m_progressCb;    // Progress callback to call
    const char      *m_swTitel;      // Current software title of device
    const char      *m_swVersion;    // Current software version of device
    SWOTA_Updater   *m_updater;      // Updater implementation used to write software data
    uint8_t         m_retries;       // Maximum amount of retries for a single chunk to be downloaded and flashes successfully
    uint16_t        m_size;          // Size of chunks the software data will be split into
    uint64_t        m_timeout;       // How long we wait for each chunck to arrive before declaring it as failed
};

#endif // THINGSBOARD_ENABLE_SWOTA

#endif // SWOTA_Update_Callback_h
