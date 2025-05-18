// This file defines the mysys options
// which control the behavior of the system construction.
#pragma once

// KV OPTIONS
// #define MTSYS_OPTION_KVUNCYNC 0
const int SERVICE_IPC_QUEUE_SIZE = 456;
#define MTSYS_KV_WAITUS 1024

// MEM OPTIONS
#define MTSYS_OPTION_LCMEMORY 1
#define MTSYS_OPTION_CACHE 1
#define MTSYS_OPTION_COMBINE 1
