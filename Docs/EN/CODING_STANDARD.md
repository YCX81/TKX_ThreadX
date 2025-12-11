# Coding Standard

**Project**: TKX_ThreadX
**Compliance**: IEC 61508 SIL 2 / ISO 13849 PL d / MISRA-C:2012
**Version**: 1.0.0

---

## Table of Contents

1. [File Structure](#1-file-structure)
2. [Naming Conventions](#2-naming-conventions)
3. [Comment Standards](#3-comment-standards)
4. [Code Formatting](#4-code-formatting)
5. [Safety Programming Rules](#5-safety-programming-rules)
6. [MISRA-C Key Rules](#6-misra-c-key-rules)
7. [Prohibited Patterns](#7-prohibited-patterns)

---

## 1. File Structure

### 1.1 Header File Template

```c
/**
 ******************************************************************************
 * @file    module_name.h
 * @brief   Brief description of the module
 * @author  Author Name
 * @version V1.0.0
 ******************************************************************************
 * @attention
 *
 * Detailed description and notes
 * Target: STM32F407VGT6
 * Compliance: IEC 61508 SIL 2 / ISO 13849 PL d
 *
 ******************************************************************************
 */

#ifndef __MODULE_NAME_H
#define __MODULE_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported defines ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __MODULE_NAME_H */
```

### 1.2 Source File Template

```c
/**
 ******************************************************************************
 * @file    module_name.c
 * @brief   Brief description of the module
 * @author  Author Name
 * @version V1.0.0
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "module_name.h"

/* Private defines -----------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* ============================================================================
 * Public Functions
 * ============================================================================*/

/* ============================================================================
 * Private Functions
 * ============================================================================*/
```

---

## 2. Naming Conventions

### 2.1 File Naming

| Module Type | Format | Example |
|-------------|--------|---------|
| Safety Module | `safety_xxx.c/h` | `safety_core.c` |
| BSP Module | `bsp_xxx.c/h` | `bsp_debug.c` |
| App Module | `app_xxx.c/h` | `app_main.c` |
| Service Module | `svc_xxx.c/h` | `svc_params.c` |

### 2.2 Function Naming

```c
// Public functions: Module_Function()
safety_status_t Safety_Watchdog_Init(void);
void BSP_Debug_Printf(const char *fmt, ...);
shared_status_t App_PreInit(void);

// Private functions: Use static, lowercase or mixed
static void update_state_machine(void);
static bool check_token_validity(uint8_t token);
```

### 2.3 Variable Naming

```c
// Static variables: s_ prefix
static safety_context_t s_safety_ctx;
static bool s_initialized = false;
static uint32_t s_error_count = 0;

// Global variables: g_ prefix (avoid if possible)
extern IWDG_HandleTypeDef g_hiwdg;

// Local variables: lowercase with underscores
uint32_t current_time = 0;
bool is_valid = false;
```

### 2.4 Type Definitions

```c
// Type names: lowercase + _t suffix
typedef struct {
    safety_state_t state;
    safety_error_t last_error;
    uint32_t error_count;
} safety_context_t;

// Enum values: UPPERCASE, explicit values, U suffix
typedef enum {
    SAFETY_STATE_INIT           = 0x00U,
    SAFETY_STATE_STARTUP_TEST   = 0x01U,
    SAFETY_STATE_NORMAL         = 0x02U,
    SAFETY_STATE_DEGRADED       = 0x03U,
    SAFETY_STATE_SAFE           = 0x04U,
    SAFETY_STATE_ERROR          = 0xFFU
} safety_state_t;
```

### 2.5 Macros and Constants

```c
// Constants: UPPERCASE + U/UL suffix
#define SAFETY_THREAD_STACK_SIZE    2048U
#define WDG_TOKEN_TIMEOUT_MS        800U
#define FLOW_SIGNATURE_SEED         0x5A5A5A5AUL
#define BOOT_CONFIG_MAGIC           0xC0F16000UL

// Bit masks: explicit hex values
#define WDG_TOKEN_SAFETY_THREAD     0x01U
#define WDG_TOKEN_APP_THREAD        0x02U
#define WDG_TOKEN_ALL               0x03U
```

---

## 3. Comment Standards

### 3.1 Doxygen Comments

```c
/**
 * @brief Initialize the watchdog management module
 * @note  Call this before starting the RTOS kernel
 * @retval SAFETY_OK if successful
 * @retval SAFETY_ERROR if initialization failed
 */
safety_status_t Safety_Watchdog_Init(void);

/**
 * @brief Report a safety error with parameters
 * @param error Error code
 * @param param1 Additional parameter 1
 * @param param2 Additional parameter 2
 */
void Safety_ReportError(safety_error_t error, uint32_t param1, uint32_t param2);
```

### 3.2 Section Separator Comments

```c
/* ============================================================================
 * Section Name
 * ============================================================================*/

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
```

### 3.3 Inline Comments

```c
uint32_t timeout = 1000U;  /* Timeout in milliseconds */

/* Check if all tokens received within timeout window */
if (Safety_Watchdog_CheckAllTokens())
{
    Safety_Watchdog_FeedIWDG();
}
```

---

## 4. Code Formatting

### 4.1 Indentation

- **Use spaces, not tabs**
- **Indent width: 4 spaces**

### 4.2 Brace Style

```c
// Allman style: Braces on their own line
if (condition)
{
    do_something();
}
else
{
    do_other();
}

void Function(void)
{
    /* Function body */
}
```

### 4.3 Pointer Alignment

```c
// Pointer symbol on the right
char *ptr;
uint32_t *data_ptr;
void *(*callback)(void *arg);
```

### 4.4 Line Length

- **Maximum 100 characters**
- Break long expressions

```c
safety_status_t result = Safety_Params_ValidateRange(
    params->hall_offset[i],
    HALL_OFFSET_MIN,
    HALL_OFFSET_MAX
);
```

---

## 5. Safety Programming Rules

### 5.1 Input Validation

```c
safety_status_t Safety_Stack_RegisterThread(TX_THREAD *thread)
{
    /* Defensive checks */
    if (!s_initialized)
    {
        return SAFETY_ERROR;
    }

    if (thread == NULL)
    {
        return SAFETY_INVALID_PARAM;
    }

    if (s_monitored_count >= MAX_MONITORED_THREADS)
    {
        return SAFETY_ERROR;
    }

    /* Normal processing */
    /* ... */
}
```

### 5.2 Return Value Checking

```c
/* Always check return values */
safety_status_t status = Safety_Watchdog_Init();
if (status != SAFETY_OK)
{
    Safety_ReportError(SAFETY_ERR_WATCHDOG, status, 0);
    return status;
}
```

### 5.3 State Machine Rules

```c
safety_status_t Safety_SetState(safety_state_t new_state)
{
    safety_state_t old_state = s_safety_ctx.state;

    /* Validate state transition */
    switch (s_safety_ctx.state)
    {
        case SAFETY_STATE_INIT:
            /* Can only go to STARTUP_TEST or SAFE */
            if (new_state != SAFETY_STATE_STARTUP_TEST &&
                new_state != SAFETY_STATE_SAFE)
            {
                return SAFETY_ERROR;
            }
            break;

        /* Other states... */
    }

    s_safety_ctx.state = new_state;
    return SAFETY_OK;
}
```

### 5.4 Data Redundancy

```c
/* Use bit-inverted copies for critical data */
typedef struct {
    float hall_offset[3];
    float hall_gain[3];
    uint32_t hall_offset_inv[3];  /* Bit-inverted copy */
    uint32_t hall_gain_inv[3];    /* Bit-inverted copy */
} safety_params_t;

/* Verify redundancy */
bool verify_redundancy(float value, uint32_t inverted)
{
    uint32_t val_bits = *(uint32_t *)&value;
    return (val_bits ^ inverted) == 0xFFFFFFFFUL;
}
```

### 5.5 Timeout Handling

```c
/* All wait operations must have timeout */
UINT tx_status = tx_thread_sleep(SAFETY_MONITOR_PERIOD_MS);
if (tx_status != TX_SUCCESS)
{
    /* Handle timeout or error */
}

/* Hardware operation timeout */
uint32_t start_time = HAL_GetTick();
while (!flag_ready)
{
    if ((HAL_GetTick() - start_time) > TIMEOUT_MS)
    {
        return SAFETY_TIMEOUT;
    }
}
```

---

## 6. MISRA-C Key Rules

### 6.1 Mandatory Rules

| Rule | Description |
|------|-------------|
| Rule 1.3 | No undefined or critical unspecified behavior |
| Rule 10.1 | Operands shall not be of inappropriate types |
| Rule 11.3 | Cast shall not be performed between pointer types |
| Rule 12.2 | Right operand of shift operator shall be appropriate |
| Rule 13.5 | Right operand of logical operator shall have no side effects |
| Rule 14.3 | Controlling expressions shall not be invariant |
| Rule 17.7 | Return value of function shall be used |
| Rule 21.3 | Memory functions of stdlib.h shall not be used |

### 6.2 Advisory Rules

| Rule | Description |
|------|-------------|
| Rule 2.2 | No dead code |
| Rule 8.7 | Functions should have internal linkage if used in one unit |
| Rule 15.5 | Function should have single point of exit |
| Rule 18.4 | +, -, +=, -= shall not be applied to pointers |

---

## 7. Prohibited Patterns

### 7.1 Prohibited Functions

```c
/* ❌ Prohibited */
malloc(), free(), realloc(), calloc()  /* Dynamic allocation */
sprintf(), vsprintf()                   /* No bounds checking */
gets()                                  /* Unsafe input */
strcpy(), strcat()                      /* No bounds checking */

/* ✅ Use alternatives */
snprintf(), vsnprintf()                 /* With bounds checking */
strncpy(), strncat()                    /* With length limit */
```

### 7.2 Prohibited Patterns

```c
/* ❌ Prohibited: Uninitialized variables */
int value;
use_value(value);

/* ❌ Prohibited: Magic numbers */
if (status == 3)  /* What is 3? */

/* ❌ Prohibited: goto statement */
goto error_handler;

/* ❌ Prohibited: Infinite loop without exit */
while (1)
{
    /* No break or return */
}

/* ❌ Prohibited: Side effects in logical expressions */
if (x++ && y--)
```

### 7.3 Recommended Patterns

```c
/* ✅ Initialize all variables */
int value = 0;

/* ✅ Use named constants */
if (status == STATUS_TIMEOUT)

/* ✅ Use explicit loop control */
while (running)
{
    if (should_exit)
    {
        break;
    }
    /* ... */
}

/* ✅ Separate side effects and logic */
x++;
y--;
if (x != 0 && y != 0)
```

---

## Version History

| Version | Date | Description |
|---------|------|-------------|
| 1.0.0 | 2025-12-10 | Initial version |
