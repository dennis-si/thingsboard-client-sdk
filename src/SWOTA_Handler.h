#ifndef SWOTA_Handler_h
#define SWOTA_Handler_h

// Local include.
#include "Configuration.h"

#if THINGSBOARD_ENABLE_SWOTA

// Local include.
#include "Callback_Watchdog.h"
#include "HashGenerator.h"
#include "Helper.h"
#include "SWOTA_Update_Callback.h"
#include "OTA_Failure_Response.h"
#include "SWOTA_Updater.h"


/// ---------------------------------
/// Constant strings in flash memory.
/// ---------------------------------
// Software data keys.
#if THINGSBOARD_ENABLE_PROGMEM
constexpr char SW_STATE_DOWNLOADING[] PROGMEM = "DOWNLOADING";
constexpr char SW_STATE_DOWNLOADED[] PROGMEM = "DOWNLOADED";
constexpr char SW_STATE_VERIFIED[] PROGMEM = "VERIFIED";
constexpr char SW_STATE_UPDATING[] PROGMEM = "UPDATING";
constexpr char SW_STATE_UPDATED[] PROGMEM = "UPDATED";
constexpr char SW_STATE_FAILED[] PROGMEM = "FAILED";
#else
constexpr char SW_STATE_DOWNLOADING[] = "DOWNLOADING";
constexpr char SW_STATE_DOWNLOADED[] = "DOWNLOADED";
constexpr char SW_STATE_VERIFIED[] = "VERIFIED";
constexpr char SW_STATE_UPDATING[] = "UPDATING";
constexpr char SW_STATE_UPDATED[] = "UPDATED";
constexpr char SW_STATE_FAILED[] = "FAILED";
#endif // THINGSBOARD_ENABLE_PROGMEM

// Log messages.
#if THINGSBOARD_ENABLE_PROGMEM
constexpr char UNABLE_TO_REQUEST_CHUNCKS[] PROGMEM = "Unable to request software chunk";
constexpr char RECEIVED_UNEXPECTED_CHUNK[] PROGMEM = "Received chunk (%u), not the same as requested chunk (%u)";
constexpr char ERROR_UPDATE_BEGIN[] PROGMEM = "Failed to initalize flash updater";
constexpr char ERROR_UPDATE_WRITE[] PROGMEM = "Only wrote (%u) bytes of binary data to flash memory instead of expected (%u)";
constexpr char UPDATING_HASH_FAILED[] PROGMEM = "Updating hash failed";
constexpr char ERROR_UPDATE_END[] PROGMEM = "Error (%u) during flash updater not all bytes written";
constexpr char CHKS_VER_FAILED[] PROGMEM = "Checksum verification failed";
constexpr char SW_CHUNK[] PROGMEM = "Receive chunk (%u), with size (%u) bytes";
constexpr char HASH_ACTUAL[] PROGMEM = "(%s) actual checksum: (%s)";
constexpr char HASH_EXPECTED[] PROGMEM = "(%s) expected checksum: (%s)";
constexpr char CHKS_VER_SUCCESS[] PROGMEM = "Checksum is the same as expected";
constexpr char SW_UPDATE_ABORTED[] PROGMEM = "Software update aborted";
constexpr char SW_UPDATE_SUCCESS[] PROGMEM = "Update success";
#else
//constexpr char UNABLE_TO_REQUEST_CHUNCKS[] = "Unable to request software chunk";
//constexpr char RECEIVED_UNEXPECTED_CHUNK[] = "Received chunk (%u), not the same as requested chunk (%u)";
//constexpr char ERROR_UPDATE_BEGIN[] = "Failed to initalize flash updater";
//constexpr char ERROR_UPDATE_WRITE[] = "Only wrote (%u) bytes of binary data to flash memory instead of expected (%u)";
//constexpr char UPDATING_HASH_FAILED[] = "Updating hash failed";
//constexpr char ERROR_UPDATE_END[] = "Error during flash updater not all bytes written";
//constexpr char CHKS_VER_FAILED[] = "Checksum verification failed";
constexpr char SW_CHUNK[] = "Receive chunk (%u), with size (%u) bytes";
//constexpr char HASH_ACTUAL[] = "(%s) actual checksum: (%s)";
//constexpr char HASH_EXPECTED[] = "(%s) expected checksum: (%s)";
//constexpr char CHKS_VER_SUCCESS[] = "Checksum is the same as expected";
constexpr char SW_UPDATE_ABORTED[] = "Software update aborted";
constexpr char SW_UPDATE_SUCCESS[] = "Update success";
#endif // THINGSBOARD_ENABLE_PROGMEM


/// @brief Handles the complete processing of received binary software data, including flashing it onto the device,
/// creating a hash of the received data and in the end ensuring that the complete OTA software was flashes successfully and that the hash is the one we initally received
/// @tparam Logger Logging class that should be used to print messages generated by internal processes
template<typename Logger>
class SWOTA_Handler {
  public:
    /// @brief Constructor
    /// @param publish_callback Callback that is used to request the software chunk of the software binary with the given chunk number
    /// @param send_sw_state_callback Callback that is used to send information about the current state of the over the air update
    /// @param finish_callback Callback that is called once the update has been finished and the user should be informed of the failure or success of the over the air update
    inline SWOTA_Handler(std::function<bool(const size_t&)> publish_callback, std::function<bool(const char *, const char *)> send_sw_state_callback, std::function<bool(void)> finish_callback)
        : m_sw_callback(nullptr)
        , m_publish_callback(publish_callback)
        , m_send_sw_state_callback(send_sw_state_callback)
        , m_finish_callback(finish_callback)
        , m_sw_size(0U)
        , m_sw_algorithm()
        , m_sw_checksum()
        , m_sw_checksum_algorithm()
        , m_hash()
        , m_total_chunks(0U)
        , m_requested_chunks(0U)
        , m_retries(0U)
        , m_watchdog(std::bind(&SWOTA_Handler::Handle_Request_Timeout, this))
    {
      // Nothing to do
    }

    /// @brief Starts the software update with requesting the first software packet and initalizes the underlying needed components
    /// @param sw_callback Callback method that contains configuration information, about the over the air update
    /// @param sw_size Complete size of the software binary that will be downloaded and flashed onto this device
    /// @param sw_algorithm String of the algorithm type used to hash the software binary
    /// @param sw_checksum Checksum of the complete software binary, should be the same as the actually written data in the end
    /// @param sw_checksum_algorithm Algorithm type used to hash the software binary
    inline void Start_Software_Update(const SWOTA_Update_Callback *sw_callback, const size_t& sw_size, const std::string& sw_algorithm, const std::string& sw_checksum, const mbedtls_md_type_t& sw_checksum_algorithm) {
        m_sw_callback = sw_callback;
        m_sw_size = sw_size;
        m_total_chunks = (m_sw_size / m_sw_callback->Get_Chunk_Size()) + 1U;
        m_sw_algorithm = sw_algorithm;
        m_sw_checksum = sw_checksum;
        m_sw_checksum_algorithm = sw_checksum_algorithm;
        m_sw_updater = m_sw_callback->Get_Updater();

        if (!m_publish_callback || !m_send_sw_state_callback || !m_finish_callback || !m_sw_updater) {
          Logger::log(OTA_CB_IS_NULL);
          (void)m_send_sw_state_callback(SW_STATE_FAILED, OTA_CB_IS_NULL);
            return Handle_Failure(OTA_Failure_Response::RETRY_NOTHING);
        }
        Request_First_Software_Packet();
    }

    /// @brief Stops the software update completly and informs that user that the update has failed because it has been aborted, ongoing communication is discarded.
    /// Be aware the written partition is not erased so the already written binary software data still remains in the flash partition,
    /// shouldn't really matter, because if we start the update process again the partition will be overwritten anyway and a partially written software will not be bootable
    inline void Stop_Software_Update() {
        m_watchdog.detach();
        m_sw_updater->reset();
        Logger::log(SW_UPDATE_ABORTED);
        (void)m_send_sw_state_callback(SW_STATE_FAILED, SW_UPDATE_ABORTED);
        Handle_Failure(OTA_Failure_Response::RETRY_NOTHING);
        m_sw_callback = nullptr;
    }

    /// @brief Uses the given software packet data and process it. Starting with writing the given amount of bytes of the packet data into flash memory and
    /// into a hash function that will be used to compare the expected complete binary file and the actually received binary file
    /// @param current_chunk Index of the chunk we recieved the binary data for
    /// @param payload Software packet data of the current chunk
    /// @param total_bytes Amount of bytes in the current software packet data
    inline void Process_Software_Packet(const size_t& current_chunk, uint8_t *payload, const size_t& total_bytes) {
        (void)m_send_sw_state_callback(SW_STATE_DOWNLOADING, nullptr);

        if (current_chunk != m_requested_chunks) {
          char message[Helper::detectSize(RECEIVED_UNEXPECTED_CHUNK, current_chunk, m_requested_chunks)];
          snprintf_P(message, sizeof(message), RECEIVED_UNEXPECTED_CHUNK, current_chunk, m_requested_chunks);
          Logger::log(message);
          return;
        }

        m_watchdog.detach();

        char message[Helper::detectSize(SW_CHUNK, current_chunk, total_bytes)];
        snprintf_P(message, sizeof(message), SW_CHUNK, current_chunk, total_bytes);
        Logger::log(message);

        if (current_chunk == 0U) {
            // Initialize Flash
            if (!m_sw_updater->begin(m_sw_size)) {
              Logger::log(ERROR_UPDATE_BEGIN);
              (void)m_send_sw_state_callback(SW_STATE_FAILED, ERROR_UPDATE_BEGIN);
              return Handle_Failure(OTA_Failure_Response::RETRY_UPDATE);
            }
        }

        // Write received binary data to flash partition
        const size_t written_bytes = m_sw_updater->write(payload, total_bytes);
        if (written_bytes != total_bytes) {
            char message[Helper::detectSize(ERROR_UPDATE_WRITE, written_bytes, total_bytes)];
            snprintf_P(message, sizeof(message), ERROR_UPDATE_WRITE, written_bytes, total_bytes);
            Logger::log(message);
            (void)m_send_sw_state_callback(SW_STATE_FAILED, message);
            return Handle_Failure(OTA_Failure_Response::RETRY_UPDATE);
        }

        // Update value only if writing to flash was a success
        if (!m_hash.update(payload, total_bytes)) {
            Logger::log(UPDATING_HASH_FAILED);
            (void)m_send_sw_state_callback(SW_STATE_FAILED, UPDATING_HASH_FAILED);
            return Handle_Failure(OTA_Failure_Response::RETRY_UPDATE);
        }

        m_requested_chunks = current_chunk + 1;
        m_sw_callback->Call_Progress_Callback<Logger>(m_requested_chunks, m_total_chunks);

        // Ensure to check if the update was cancelled during the progress callback,
        // if it was the callback variable was reset and there is no need to request the next software packet
        if (m_sw_callback == nullptr) {
          return;
        }

        // Reset retries as the current chunk has been downloaded and handled successfully
        m_retries = m_sw_callback->Get_Chunk_Retries();
        Request_Next_Software_Packet();
    }

  private:
    const SWOTA_Update_Callback *m_sw_callback;                                 // Callback method that contains configuration information, about the over the air update
    std::function<bool(const size_t&)> m_publish_callback;                    // Callback that is used to request the software chunk of the software binary with the given chunk number
    std::function<bool(const char *, const char *)> m_send_sw_state_callback; // Callback that is used to send information about the current state of the over the air update
    std::function<bool(void)> m_finish_callback;                              // Callback that is called once the update has been finished and the user should be informed of the failure or success of the over the air update
    size_t m_sw_size;                                                         // Total size of the software binary we will receive. Allows for a binary size of up to theoretically 4 GB
    std::string m_sw_algorithm;                                               // String of the algorithm type used to hash the software binary
    std::string m_sw_checksum;                                                // Checksum of the complete software binary, should be the same as the actually written data in the end
    mbedtls_md_type_t m_sw_checksum_algorithm;                                // Algorithm type used to hash the software binary
    SWOTA_Updater *m_sw_updater;                                                   // Interface implementation that writes received software binary data onto the given device
    HashGenerator m_hash;                                                     // Class instance that allows to generate a hash from received software binary data
    size_t m_total_chunks;                                                    // Total amount of chunks that need to be received to get the complete software binary
    size_t m_requested_chunks;                                                // Amount of successfully requested and received software binary chunks
    uint8_t m_retries;                                                        // Amount of request retries we attempt for each chunk, increasing makes the connection more stable
    Callback_Watchdog m_watchdog;                                             // Class instances that allows to timeout if we do not receive a response for a requested chunk in the given time

    /// @brief Restarts or starts the software update and its needed components and then requests the first software chunk
    inline void Request_First_Software_Packet() {
        m_requested_chunks = 0U;
        m_retries = m_sw_callback->Get_Chunk_Retries();
        m_hash.start(m_sw_checksum_algorithm);
        m_watchdog.detach();
        m_sw_updater->reset(); // edit by DD was ->reset()
        Request_Next_Software_Packet();
    }

    /// @brief Requests the next software chunk of the OTA software if there are any left
    /// and starts the timer that ensures we request the same chunk again if we have not received a response yet
    inline void Request_Next_Software_Packet() {
        // Check if we have already requested and handled the last remaining chunk
        if (m_requested_chunks >= m_total_chunks) {
            Finish_Software_Update();   
            return;
        }

        if (!m_publish_callback(m_requested_chunks)) {
          Logger::log(UNABLE_TO_REQUEST_CHUNCKS);
          (void)m_send_sw_state_callback(SW_STATE_FAILED, UNABLE_TO_REQUEST_CHUNCKS);
        }

        // Watchdog gets started no matter if publishing request was successful or not in hopes,
        // that after the given timeout the callback calls this method again and can then publish the request successfully.
        m_watchdog.once(m_sw_callback->Get_Timeout());
    }

    /// @brief Completes the software update, which consists of checking the complete hash of the software binary if the initally received value,
    /// both should be the same and if that is not the case that means that we received invalid software binary data and have to restart the update.
    /// If checking the hash was successfull we attempt to finish flashing the ota partition and then inform the user that the update was successfull
    inline void Finish_Software_Update() {
        (void)m_send_sw_state_callback(SW_STATE_DOWNLOADED, nullptr);

        const std::string calculated_hash = m_hash.get_hash_string();
        char actual[JSON_STRING_SIZE(strlen(HASH_ACTUAL)) + JSON_STRING_SIZE(m_sw_algorithm.size()) + JSON_STRING_SIZE(calculated_hash.size())];
        snprintf_P(actual, sizeof(actual), HASH_ACTUAL, m_sw_algorithm.c_str(), calculated_hash.c_str());
        Logger::log(actual);

        char expected[JSON_STRING_SIZE(strlen(HASH_EXPECTED)) + JSON_STRING_SIZE(m_sw_algorithm.size()) + JSON_STRING_SIZE(m_sw_checksum.size())];
        snprintf_P(expected, sizeof(expected), HASH_EXPECTED, m_sw_algorithm.c_str(), m_sw_checksum.c_str());
        Logger::log(expected);

        // Check if the initally received checksum is the same as the one we calculated from the received binary data,
        // if not we assume the binary data has been changed or not completly downloaded --> Software update failed
        if (m_sw_checksum.compare(calculated_hash) != 0) {
            Logger::log(CHKS_VER_FAILED);
            (void)m_send_sw_state_callback(SW_STATE_FAILED, CHKS_VER_FAILED);
            return Handle_Failure(OTA_Failure_Response::RETRY_UPDATE);
        }

        Logger::log(CHKS_VER_SUCCESS);

        if (!m_sw_updater->end()) {
            Logger::log(ERROR_UPDATE_END);
            (void)m_send_sw_state_callback(SW_STATE_FAILED, ERROR_UPDATE_END);
            return Handle_Failure(OTA_Failure_Response::RETRY_UPDATE);
        }

        Logger::log(SW_UPDATE_SUCCESS);
        (void)m_send_sw_state_callback(SW_STATE_UPDATING, nullptr);

        m_sw_callback->Call_Callback<Logger>(true);
        (void)m_finish_callback();
    }

    /// @brief Handles errors with the received failure response so that the software update can regenerate from any possible issue.
    /// Will only execute the given failure response as long as there are still retries remaining, if there are not any further issue will cause the update to be aborted
    /// @param failure_response Possible response to a failure that the method should handle
    inline void Handle_Failure(const OTA_Failure_Response& failure_response) {
      if (m_retries <= 0) {
          m_sw_callback->Call_Callback<Logger>(false);
          (void)m_finish_callback();
          return;
      }

      // Decrease the amount of retries of downloads for the current chunk,
      // reset as soon as the next chunk has been received and handled successfully
      m_retries--;

      switch (failure_response) {
        case OTA_Failure_Response::RETRY_CHUNK:
          Request_Next_Software_Packet();
          break;
        case OTA_Failure_Response::RETRY_UPDATE:
          Request_First_Software_Packet();
          break;
        case OTA_Failure_Response::RETRY_NOTHING:
          m_sw_callback->Call_Callback<Logger>(false);
          (void)m_finish_callback();
          break;
        default:
          // Nothing to do
          break;
      }
    }

    /// @brief Callback that will be called if we did not receive the software chunk response in the given timeout time
    inline void Handle_Request_Timeout() {
        Handle_Failure(OTA_Failure_Response::RETRY_CHUNK);
    }
};

#endif // THINGSBOARD_ENABLE_SWOTA

#endif // SWOTA_Handler_h
