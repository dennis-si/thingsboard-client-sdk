// Header include.
#include "SWOTA_Update_Callback.h"

#if THINGSBOARD_ENABLE_SWOTA

SWOTA_Update_Callback::SWOTA_Update_Callback() :
    SWOTA_Update_Callback(nullptr, nullptr, nullptr, nullptr, nullptr)
{
    // Nothing to do
}

SWOTA_Update_Callback::SWOTA_Update_Callback(function endCb, const char *currSwTitle, const char *currSwVersion, SWOTA_Updater *updater, const uint8_t &chunkRetries, const uint16_t &chunkSize, const uint64_t &timeout) :
    SWOTA_Update_Callback(nullptr, endCb, currSwTitle, currSwVersion, updater, chunkRetries, chunkSize, timeout)
{
    // Nothing to do
}

SWOTA_Update_Callback::SWOTA_Update_Callback(progressFn progressCb, function endCb, const char *currSwTitle, const char *currSwVersion, SWOTA_Updater *updater, const uint8_t &chunkRetries, const uint16_t &chunkSize, const uint64_t &timeout) :
    Callback(endCb, SW_OTA_CB_IS_NULL),
    m_progressCb(progressCb),
    m_swTitel(currSwTitle),
    m_swVersion(currSwVersion),
    m_updater(updater),
    m_retries(chunkRetries),
    m_size(chunkSize),
    m_timeout(timeout)
{
    // Nothing to do
}

void SWOTA_Update_Callback::Set_Progress_Callback(progressFn progressCb) {
    m_progressCb = progressCb;
}

const char* SWOTA_Update_Callback::Get_Software_Title() const {
    return m_swTitel;
}

void SWOTA_Update_Callback::Set_Software_Title(const char *currSwTitle) {
    m_swTitel = currSwTitle;
}

const char* SWOTA_Update_Callback::Get_Software_Version() const {
    return m_swVersion;
}

void SWOTA_Update_Callback::Set_Software_Version(const char *currSwVersion) {
    m_swVersion = currSwVersion;
}

SWOTA_Updater* SWOTA_Update_Callback::Get_Updater() const {
    return m_updater;
}

void SWOTA_Update_Callback::Set_Updater(SWOTA_Updater* updater) {
    m_updater = updater;
}

const uint8_t& SWOTA_Update_Callback::Get_Chunk_Retries() const {
  return m_retries;
}

void SWOTA_Update_Callback::Set_Chunk_Retries(const uint8_t &chunkRetries) {
    m_retries = chunkRetries;
}

const uint16_t& SWOTA_Update_Callback::Get_Chunk_Size() const {
  return m_size;
}

void SWOTA_Update_Callback::Set_Chunk_Size(const uint16_t &chunkSize) {
    m_size = chunkSize;
}

const uint64_t& SWOTA_Update_Callback::Get_Timeout() const {
    return m_timeout;
}

void SWOTA_Update_Callback::Set_Timeout(const uint64_t &timeout_microseconds) {
    m_timeout = timeout_microseconds;
}

#endif // THINGSBOARD_ENABLE_SWOTA