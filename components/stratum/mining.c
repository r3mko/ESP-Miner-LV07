#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "esp_log.h"
#include "mining.h"
#include "utils.h"

static const char *TAG = "mining";

void calculate_coinbase_tx_hash_bin(const uint8_t *prefix, size_t prefix_len,
                                    const uint8_t *extranonce_prefix, size_t ep_len,
                                    const uint8_t *extranonce_2, size_t e2_len,
                                    const uint8_t *suffix, size_t suffix_len,
                                    uint8_t dest[32])
{
    size_t total_len = prefix_len + ep_len + e2_len + suffix_len;
    uint8_t stack_buf[1024];
    uint8_t *buf = (total_len <= sizeof(stack_buf)) ? stack_buf : malloc(total_len);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for coinbase tx (%zu bytes)", total_len);
        if (dest) memset(dest, 0, 32);
        return;
    }

    size_t offset = 0;
    if (prefix && prefix_len > 0) {
        memcpy(buf + offset, prefix, prefix_len);
        offset += prefix_len;
    }
    if (extranonce_prefix && ep_len > 0) {
        memcpy(buf + offset, extranonce_prefix, ep_len);
        offset += ep_len;
    }
    if (extranonce_2 && e2_len > 0) {
        memcpy(buf + offset, extranonce_2, e2_len);
        offset += e2_len;
    }
    if (suffix && suffix_len > 0) {
        memcpy(buf + offset, suffix, suffix_len);
        offset += suffix_len;
    }

    double_sha256_bin(buf, total_len, dest);
    if (buf != stack_buf) {
        free(buf);
    }
}

void calculate_merkle_root_hash(const uint8_t coinbase_tx_hash[32], const uint8_t merkle_branches[][32], const int num_merkle_branches, uint8_t dest[32])
{
    uint8_t both_merkles[64];
    memcpy(both_merkles, coinbase_tx_hash, 32);
    for (int i = 0; i < num_merkle_branches; i++) {
        memcpy(both_merkles + 32, merkle_branches[i], 32);
        double_sha256_bin(both_merkles, 64, both_merkles);
    }

    memcpy(dest, both_merkles, 32);
}

bool mining_nonce_hash(const asic_job_t *job, const uint32_t nonce, const uint32_t rolled_version, uint8_t hash[32])
{
    uint8_t header[80];
    asic_job_header(job, nonce, rolled_version, header);
    return double_sha256_bin(header, sizeof(header), hash);
}

uint32_t increment_bitmask(const uint32_t value, const uint32_t mask)
{
    // Carry across gaps in the mask while preserving every unmasked bit.
    return (value & ~mask) | (((value | ~mask) + 1U) & mask);
}
