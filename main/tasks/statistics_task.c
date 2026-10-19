#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include <esp_heap_caps.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "statistics_task.h"
#include "global_state.h"
#include "nvs_config.h"
#include "connect.h"

#define DEFAULT_POLL_RATE 1000

static const char * TAG = "statistics_task";

const char * const STATS_LABELS[SRC_NONE] = {
    [SRC_HASHRATE] = "hashrate",
    [SRC_HASHRATE_1m] = "hashrate_1m",
    [SRC_HASHRATE_10m] = "hashrate_10m",
    [SRC_HASHRATE_1h] = "hashrate_1h",
    [SRC_ERROR_PERCENTAGE] = "errorPercentage",
    [SRC_ASIC_TEMP] = "asicTemp",
    [SRC_ASIC_TEMP1] = "asicTemp1",
    [SRC_ASIC_TEMP2] = "asicTemp2",
    [SRC_VR_TEMP] = "vrTemp",
    [SRC_ASIC_VOLTAGE] = "asicVoltage",
    [SRC_VOLTAGE] = "voltage",
    [SRC_POWER] = "power",
    [SRC_CURRENT] = "current",
    [SRC_FAN_SPEED] = "fanSpeed",
    [SRC_FAN_RPM] = "fanRpm",
    [SRC_FAN2_RPM] = "fan2Rpm",
    [SRC_WIFI_RSSI] = "wifiRssi",
    [SRC_FREE_HEAP] = "freeHeap",
    [SRC_RESPONSE_TIME_MIN] = "responseTimeMin",
    [SRC_RESPONSE_TIME_MAX] = "responseTimeMax",
};

static StatisticsDataPtr statisticsBuffer;
static uint16_t statisticsDataSize;
static pthread_mutex_t statisticsDataLock = PTHREAD_MUTEX_INITIALIZER;

static const uint16_t maxDataCount = MAX_STATISTICS_COUNT;

static void formatStatisticFloat(char *token, float value)
{
    if (isfinite(value)) {
        snprintf(token, STATS_TOKEN_LEN, "%.2f", value);
    } else {
        strcpy(token, "null");
    }
}

void createStatisticsBuffer()
{
    if (NULL == statisticsBuffer) {
        pthread_mutex_lock(&statisticsDataLock);

        if (NULL == statisticsBuffer) {
            statisticsBuffer = (StatisticsDataPtr)heap_caps_malloc(sizeof(struct StatisticsData) * maxDataCount, MALLOC_CAP_SPIRAM);
            if (NULL == statisticsBuffer) {
                ESP_LOGW(TAG, "Not enough memory for the statistics data buffer!");
            }
        }

        pthread_mutex_unlock(&statisticsDataLock);
    }
}

void removeStatisticsBuffer()
{
    if (NULL != statisticsBuffer) {
        pthread_mutex_lock(&statisticsDataLock);

        if (NULL != statisticsBuffer) {
            heap_caps_free(statisticsBuffer);

            statisticsBuffer = NULL;
            statisticsDataSize = 0;
        }

        pthread_mutex_unlock(&statisticsDataLock);
    }
}

static inline void merge_min_positive(float *target, char *token, float val)
{
    if (val > 0.0f && (*target <= 0.0f || val < *target)) {
        *target = val;
        formatStatisticFloat(token, val);
    }
}

static inline void merge_max_positive(float *target, char *token, float val)
{
    if (val > 0.0f && (*target <= 0.0f || val > *target)) {
        *target = val;
        formatStatisticFloat(token, val);
    }
}

bool addStatisticData(StatisticsDataPtr data, uint16_t statsFrequency)
{
    bool result = false;

    if (NULL == data) {
        return result;
    }

    createStatisticsBuffer();

    pthread_mutex_lock(&statisticsDataLock);

    if (NULL != statisticsBuffer) {
        if (statisticsDataSize < maxDataCount) {
            statisticsBuffer[statisticsDataSize] = *data;
            statisticsDataSize++;
            result = true;
        } else {
            // Buffer is full. Determine indexToRemove using Triangle Area thinning logic.
            uint16_t indexToRemove = 0;
            const uint64_t currentSpan = data->timestamp - statisticsBuffer[0].timestamp;
            const uint64_t targetDuration = (uint64_t)maxDataCount * (uint64_t)statsFrequency * 1000;

            if (currentSpan >= targetDuration) {
                indexToRemove = 0;
            } else {
                uint16_t low = 1;
                uint16_t high = maxDataCount - 1;

                while (high - low > 1) {
                    uint64_t lowTime = statisticsBuffer[low].timestamp;
                    uint64_t highTime = statisticsBuffer[high].timestamp;
                    uint64_t midTime = (lowTime + highTime) / 2;

                    uint16_t split = low;
                    for (uint16_t i = low; i <= high; i++) {
                        uint64_t t = statisticsBuffer[i].timestamp;
                        if (t >= midTime) {
                            split = i;
                            break;
                        }
                    }

                    // Ensure progress
                    if (split == low) split++;
                    if (split > high) split = high;

                    uint16_t leftCount = split - low;
                    uint16_t rightCount = high - split + 1;

                    if (leftCount > rightCount) {
                        high = split - 1;
                    } else {
                        low = split;
                    }
                }
                indexToRemove = low;
            }

            struct StatisticsData *toRemove = &statisticsBuffer[indexToRemove];
            struct StatisticsData *successor = &statisticsBuffer[indexToRemove + 1];

            // Merge into successor before removing indexToRemove
            if (indexToRemove > 0) {
                merge_min_positive(&successor->responseTimeMin, successor->tokens[SRC_RESPONSE_TIME_MIN], toRemove->responseTimeMin);
                merge_max_positive(&successor->responseTimeMax, successor->tokens[SRC_RESPONSE_TIME_MAX], toRemove->responseTimeMax);
            }

            // Shift and append (Standard linear array shift)
            if (indexToRemove < maxDataCount - 1) {
                memmove(toRemove, successor, (maxDataCount - indexToRemove - 1) * sizeof(struct StatisticsData));
            }
            statisticsBuffer[maxDataCount - 1] = *data;
            result = true;
        }
    }

    pthread_mutex_unlock(&statisticsDataLock);

    return result;
}

void withStatisticsData(StatisticsReaderCallback cb, void *user_ctx)
{
    if (!cb) {
        return;
    }

    pthread_mutex_lock(&statisticsDataLock);

    cb(statisticsBuffer, (statisticsBuffer != NULL) ? statisticsDataSize : 0, user_ctx);

    pthread_mutex_unlock(&statisticsDataLock);
}

void statistics_task(void * pvParameters)
{
    ESP_LOGI(TAG, "Starting");

    GlobalState * GLOBAL_STATE = (GlobalState *) pvParameters;
    SystemModule * sys_module = &GLOBAL_STATE->SYSTEM_MODULE;
    PowerManagementModule * power_management = &GLOBAL_STATE->POWER_MANAGEMENT_MODULE;
    struct StatisticsData statsData = {};
    uint64_t last_shares_count = sys_module->shares_accepted + sys_module->shares_rejected;

    TickType_t taskWakeTime = xTaskGetTickCount();

    while (1) {
        const uint64_t currentTime = esp_timer_get_time() / 1000;
        const uint16_t configStatsFrequency = nvs_config_get_u16(NVS_CONFIG_STATISTICS_FREQUENCY);

        if (0 != configStatsFrequency) {
            // Record every second (DEFAULT_POLL_RATE is 1000ms)
            if (currentTime >= statsData.timestamp + 1000) {
                int8_t wifiRSSI = -90;
                get_wifi_current_rssi(&wifiRSSI);

                statsData.timestamp = currentTime;
                float chipTemperature1 = power_management->chip_temp_avg;
                float chipTemperature2 = power_management->chip_temp2_avg;
                float chipTemperature;
                bool temp1_valid = isfinite(chipTemperature1) && chipTemperature1 > 0;
                bool temp2_valid = isfinite(chipTemperature2) && chipTemperature2 > 0;
                if (temp1_valid && temp2_valid) {
                    chipTemperature = (chipTemperature1 + chipTemperature2) / 2.0f;
                } else if (temp1_valid) {
                    chipTemperature = chipTemperature1;
                } else {
                    chipTemperature = temp2_valid ? chipTemperature2 : -1.0f;
                } // average of both temps or whichever is valid, or -1 if neither is valid
                formatStatisticFloat(statsData.tokens[SRC_HASHRATE], sys_module->current_hashrate);
                formatStatisticFloat(statsData.tokens[SRC_HASHRATE_1m], sys_module->hashrate_1m);
                formatStatisticFloat(statsData.tokens[SRC_HASHRATE_10m], sys_module->hashrate_10m);
                formatStatisticFloat(statsData.tokens[SRC_HASHRATE_1h], sys_module->hashrate_1h);
                formatStatisticFloat(statsData.tokens[SRC_ERROR_PERCENTAGE], sys_module->error_percentage);
                formatStatisticFloat(statsData.tokens[SRC_ASIC_TEMP], chipTemperature);
                formatStatisticFloat(statsData.tokens[SRC_ASIC_TEMP1], chipTemperature1);
                formatStatisticFloat(statsData.tokens[SRC_ASIC_TEMP2], chipTemperature2);
                formatStatisticFloat(statsData.tokens[SRC_VR_TEMP], power_management->vr_temp);
                snprintf(statsData.tokens[SRC_ASIC_VOLTAGE], STATS_TOKEN_LEN, "%d", (int)power_management->core_voltage);
                formatStatisticFloat(statsData.tokens[SRC_VOLTAGE], power_management->voltage);
                formatStatisticFloat(statsData.tokens[SRC_POWER], power_management->power);
                formatStatisticFloat(statsData.tokens[SRC_CURRENT], power_management->current);
                formatStatisticFloat(statsData.tokens[SRC_FAN_SPEED], power_management->fan_perc);
                snprintf(statsData.tokens[SRC_FAN_RPM], STATS_TOKEN_LEN, "%u", (unsigned int)power_management->fan_rpm);
                snprintf(statsData.tokens[SRC_FAN2_RPM], STATS_TOKEN_LEN, "%u", (unsigned int)power_management->fan2_rpm);
                snprintf(statsData.tokens[SRC_WIFI_RSSI], STATS_TOKEN_LEN, "%d", (int)wifiRSSI);
                snprintf(statsData.tokens[SRC_FREE_HEAP], STATS_TOKEN_LEN, "%" PRIu32, esp_get_free_heap_size());

                uint64_t current_shares = sys_module->shares_accepted + sys_module->shares_rejected;
                if (current_shares != last_shares_count && sys_module->response_time > 0.0f) {
                    statsData.responseTimeMin = sys_module->response_time_min > 0.0f ? sys_module->response_time_min : sys_module->response_time;
                    statsData.responseTimeMax = sys_module->response_time_max > 0.0f ? sys_module->response_time_max : sys_module->response_time;
                    sys_module->last_response_time_min = statsData.responseTimeMin;
                    sys_module->last_response_time_max = statsData.responseTimeMax;
                    formatStatisticFloat(statsData.tokens[SRC_RESPONSE_TIME_MIN], statsData.responseTimeMin);
                    formatStatisticFloat(statsData.tokens[SRC_RESPONSE_TIME_MAX], statsData.responseTimeMax);
                } else {
                    statsData.responseTimeMin = 0.0f;
                    statsData.responseTimeMax = 0.0f;
                    sys_module->last_response_time_min = 0.0f;
                    sys_module->last_response_time_max = 0.0f;
                    strcpy(statsData.tokens[SRC_RESPONSE_TIME_MIN], "null");
                    strcpy(statsData.tokens[SRC_RESPONSE_TIME_MAX], "null");
                }
                last_shares_count = current_shares;
                sys_module->response_time_min = 0.0f;
                sys_module->response_time_max = 0.0f;

                addStatisticData(&statsData, configStatsFrequency);
            }
        } else {
            removeStatisticsBuffer();
        }

        vTaskDelayUntil(&taskWakeTime, DEFAULT_POLL_RATE / portTICK_PERIOD_MS); // taskWakeTime is automatically updated
    }
}
