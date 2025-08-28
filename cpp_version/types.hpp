/**
 * @file    types.hpp
 * @brief   项目通用基础类型定义头文件（C++ 版本）
 *
 * @details 本头文件用于统一定义项目中常用的基本数据类型、布尔类型、对齐方式、
 *          大小端模式、错误码以及常用函数属性宏，提升代码可读性与移植性。
 *
 *          - 简化常用整数类型（U8, S16, 等）别名定义
 *          - 定义与指针宽度相关的可移植类型（addr_t, int_addr_t）
 *          - 统一布尔类型（bool_t）
 *          - 提供常用错误码类型（error_t）
 *          - 定义平台相关的对齐与大小端枚举
 *          - 提供适用于 GCC 的函数属性宏，提升编译器优化和静态检查能力
 *
 * @note    本文件应在所有模块通用头文件中优先包含
 *
 * @author  Ash0o0
 * @date    2025-07-20
 * @version 0.01
 */

#pragma once

#include <cstdint>
#include <cstddef>  // size_t

/* ===================== 基础类型定义 ===================== */

// 常用简写类型别名（无符号）
using U8  = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
using U64 = uint64_t;

// 常用简写类型别名（有符号）
using S8  = int8_t;
using S16 = int16_t;
using S32 = int32_t;
using S64 = int64_t;

// 可移植指针相关类型
using addr_t     = uintptr_t;  // 指针宽度地址类型
using int_addr_t = intptr_t;

/* ===================== 布尔类型定义（可选） ===================== */

#ifndef BOOL_T_DEFINED
#define BOOL_T_DEFINED
enum class bool_t : uint8_t {
    FALSE_E = 0,
    TRUE_E  = 1
};
#endif

constexpr bool TRUE_VAL  = true;
constexpr bool FALSE_VAL = false;

/* ===================== 对齐与大小端定义 ===================== */

enum class align_t : uint8_t {
    ALIGN_OK = 0,
    ALIGN_ERR
};

enum class endian_t : uint8_t {
    LE_MODE = 0,
    BE_MODE
};

/* ===================== 错误与异常定义 ===================== */

enum class error_t : uint8_t {
    ERR_NONE = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
    ERR_NO_MEMORY,
    ERR_NOT_FOUND,
    ERR_BUSY,
    ERR_UNSUPPORTED,
    ERR_HARDWARE_FAIL,
    ERR_UNKNOWN
};

/* ===================== 通用函数属性宏 ===================== */

#define __PRINTF_FMT      __attribute__((format(printf, 1, 2)))
#define __INLINE_ALWAYS   __attribute__((always_inline)) inline
#define __DEPRECATED      __attribute__((deprecated))
#define __CONST_FUNC      __attribute__((const))
