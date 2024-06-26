#ifndef USER_TA_HEADER_DEFINES_H
#define USER_TA_HEADER_DEFINES_H

#include <video_tee_ta.h>

#define TA_UUID TA_VIDEO_TEE_UUID

#define TA_FLAGS TA_FLAG_SINGLE_INSTANCE

/* Stack and heap size for TA */
#define TA_STACK_SIZE (2 * 1024)
#define TA_DATA_SIZE  (20 * 1024 * 1024)
// #define TA_DATA_SIZE  104857600

#endif // !USER_TA_HEADER_DEFINES_H
