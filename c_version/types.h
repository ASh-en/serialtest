/**
 * @file    types.h
 * @brief   项目通用基础类型定义头文件
 *
 * @details 本头文件用于统一定义项目中常用的基本数据类型、布尔类型、对齐方式、
 *          大小端模式、错误码以及常用函数属性宏，提升代码可读性与移植性。
 *
 *          - 简化常用整数类型（U8, S16, 等）别名定义
 *          - 定义与指针宽度相关的可移植类型（addr_t, int_addr_t）
 *          - 统一布尔类型（bool_t、TRUE/FALSE 宏）
 *          - 提供常用错误码类型（error_t）
 *          - 定义平台相关的对齐与大小端枚举
 *          - 提供适用于 GCC 的函数属性宏，提升编译器优化和静态检查能力
 *
 * @note    本文件应在所有模块通用头文件中优先包含
 *
 * @author  Ash0o0
 * @date    2025-07-20
 * @version 0.00
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>  // for size_t
#include <stdbool.h> // 

/* ===================== 基础类型定义 ===================== */

// 常用简写类型别名（无符号）
typedef uint8_t   U8;
typedef uint16_t  U16;
typedef uint32_t  U32;
typedef uint64_t  U64;

// 常用简写类型别名（有符号）
typedef int8_t    S8;
typedef int16_t   S16;
typedef int32_t   S32;
typedef int64_t   S64;

// 可移植指针相关类型
typedef uintptr_t addr_t;   // 指针宽度地址类型
typedef intptr_t  int_addr_t;

/* ===================== 布尔类型定义（可选） ===================== */

#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef BOOL_T_DEFINED
#define BOOL_T_DEFINED
typedef enum {
    FALSE_E = 0,
    TRUE_E  = 1
} bool_t;
#endif

/* ===================== 对齐与大小端定义 ===================== */

typedef enum {
    ALIGN_OK = 0,
    ALIGN_ERR
} align_t;

typedef enum {
    LE_MODE = 0,
    BE_MODE
} endian_t;

/* ===================== 错误与异常定义 ===================== */

typedef enum {
    ERR_NONE = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
    ERR_NO_MEMORY,
    ERR_NOT_FOUND,
    ERR_BUSY,
    ERR_UNSUPPORTED,
    ERR_HARDWARE_FAIL,
    ERR_UNKNOWN
} error_t;

/* ===================== 通用函数属性宏 ===================== */

#define __PRINTF_FMT      __attribute__((format(printf, 1, 2)))
#define __INLINE_ALWAYS   __attribute__((always_inline)) inline
#define __DEPRECATED      __attribute__((deprecated))
#define __CONST_FUNC      __attribute__((const))

#ifdef __cplusplus
}
#endif

#endif /* __TYPES_H__ */
