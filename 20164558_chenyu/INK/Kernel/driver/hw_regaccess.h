
#ifndef __HW_REGACCESS__
#define __HW_REGACCESS__

#include "stdint.h"
#include "stdbool.h"

//*****************************************************************************
//
// Macro for enabling assert statements for debugging
//
//*****************************************************************************
#define NDEBUG
//*****************************************************************************
//
// Macros for hardware access
//
//*****************************************************************************
#define HWREG32(x)                                                              \
    (*((volatile uint32_t *)((uint16_t)x)))
#define HWREG16(x)                                                             \
    (*((volatile uint16_t *)((uint16_t)x)))
#define HWREG8(x)                                                             \
    (*((volatile uint8_t *)((uint16_t)x)))

//*****************************************************************************
//
// SUCCESS and FAILURE for API return value
//
//*****************************************************************************
#define STATUS_SUCCESS  0x01
#define STATUS_FAIL     0x00

#endif // #ifndef __HW_REGACCESS__
