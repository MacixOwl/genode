// This file defines the mysys options
// which control the behavior of the system construction.
#pragma once

// Design OPTIONS
#define MTSYS_OPTION_KVUNCYNC 1
#define MTSYS_OPTION_LCMEMORY 1
#define MTSYS_OPTION_CACHE 1
#define MTSYS_OPTION_COMBINE 1

// Other options
const int SERVICE_IPC_QUEUE_SIZE = 256;
#define MTSYS_KV_WAITUS 50
