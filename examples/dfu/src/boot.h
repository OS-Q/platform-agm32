#ifndef BOOT_H
#define BOOT_H

#include "alta.h"

#define BOOT_CMD_INIT  0x7F
#define BOOT_CMD_GET   0x00    /* get the version and command supported */
#define BOOT_CMD_GVR   0x01    /* get version and read protection status */
#define BOOT_CMD_GID   0x02    /* get ID */
#define BOOT_CMD_RM    0x11    /* read memory */
#define BOOT_CMD_RE    0x13    /* extended read memory */
#define BOOT_CMD_GO    0x21    /* go */
#define BOOT_CMD_WM    0x31    /* write memory */
#define BOOT_CMD_WE    0x33    /* extended write memory */
#define BOOT_CMD_WM_NS 0x32    /* no-stretch write memory */
#define BOOT_CMD_ER    0x43    /* erase */
#define BOOT_CMD_EE    0x44    /* extended erase */
#define BOOT_CMD_EE_NS 0x45    /* extended erase no-stretch */
#define BOOT_CMD_WP    0x63    /* write protect */
#define BOOT_CMD_WP_NS 0x64    /* write protect no-stretch */
#define BOOT_CMD_UW    0x73    /* write unprotect */
#define BOOT_CMD_UW_NS 0x74    /* write unprotect no-stretch */
#define BOOT_CMD_RP    0x82    /* readout protect */
#define BOOT_CMD_RP_NS 0x83    /* readout protect no-stretch */
#define BOOT_CMD_UR    0x92    /* readout unprotect */
#define BOOT_CMD_UR_NS 0x93    /* readout unprotect no-stretch */
#define BOOT_CMD_CRC   0xA1    /* compute CRC */
#define BOOT_CMD_RESET 0xA2    /* reset device */
#define BOOT_CMD_OO    0xA3    /* erase option bytes */
#define BOOT_CMD_ERR   0xFF    /* not a valid command */

#define BOOT_ACK       0x79
#define BOOT_NACK      0x1F
#define BOOT_BUSY      0x76

#ifndef BOOT_EE_EXTENDED
# define BOOT_EE_EXTENDED 2
#endif
#ifndef BOOT_RE_EXTENDED
# define BOOT_RE_EXTENDED 1
#endif
#ifndef BOOT_WE_EXTENDED
# define BOOT_WE_EXTENDED 1
#endif
#define BOOT_MAX_LEN          256
#define BOOT_MAX_LEN_EE       (BOOT_MAX_LEN*BOOT_EE_EXTENDED)
#define BOOT_MAX_LEN_RE       (BOOT_MAX_LEN*BOOT_RE_EXTENDED)
#define BOOT_MAX_LEN_WE       (BOOT_MAX_LEN*BOOT_WE_EXTENDED)
#define BOOT_OPT_LEN          128

typedef enum {
        BOOT_ERR_OK = 0,
        BOOT_ERR_UNKNOWN,      /* Generic error */
        BOOT_ERR_NACK,
        BOOT_ERR_NO_CMD,       /* Command not available in bootloader */
} BootErrType;

#define BOOT_DEVICE_ID 0x4040
#define BOOT_VERSION   0x20

typedef uint32_t BootAddr;
typedef uint32_t BootWord;
typedef uint16_t BootHalf;
typedef uint8_t  BootByte;
typedef uint16_t BootLen;

void boot();

#endif
