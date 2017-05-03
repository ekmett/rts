#pragma once

/// @file rts/attribute.hpp
/// @brief Portable support for @p __declspec and @p \__attribute__ annotations

/// @defgroup macros Macros
/// @brief Macros
///
/// @{

/// @defgroup declspecs __declspec support
/// @brief Portable support for Microsoft style @p __declspec annotations
///
/// @code{.cpp}
/// #include "rts/attribute.h"
/// @endcode
///
/// @{

// disable __declspec support on clang entirely unless we have -fms-extensions turned on
#if defined(__clang__) && !defined(_MSC_EXTENSIONS)
#define RTS_DECLSPEC(X)
#else
#define RTS_DECLSPEC(X) __declspec(X)
#endif

/// @def RTS_HAS_DECLSPEC_ATTRIBUTE(X)
/// @brief clang's @p __has_declspec_attribute, polyfilled to return 0 if not available
#ifdef __has_declspec_attribute
#define RTS_HAS_DECLSPEC_ATTRIBUTE(X) __has_declspec_attribute(X)
#else
#define RTS_HAS_DECLSPEC_ATTRIBUTE(X) 0
#endif

/// @def RTS_HAS_MS_DECLSPEC_ATTRIBUTE(X)
/// @brief modified form of clang's @p __has_declspec_attribute that always returns 1 on Microsoft Visual C++
#ifdef _MSC_VER
#define RTS_HAS_MS_DECLSPEC_ATTRIBUTE(X) 1
#else
#define RTS_HAS_MS_DECLSPEC_ATTRIBUTE(X) RTS_HAS_DECLSPEC_ATTRIBUTE(X)
#endif

/// @def RTS_DECLSPEC_NOALIAS
/// @brief portable version of @p __declspec(noalias)
#if RTS_HAS_MS_DECLSPEC_ATTRIBUTE(noalias)
#define RTS_DECLSPEC_NOALIAS RTS_DECLSPEC(noalias)
#else
#define RTS_DECLSPEC_NOALIAS
#endif

/// @def RTS_DECLSPEC_RESTRICT
/// @brief portable version of @p __declspec(restrict)
#if RTS_HAS_MS_DECLSPEC_ATTRIBUTE(restrict)
#define RTS_DECLSPEC_RESTRICT RTS_DECLSPEC(restrict)
#else
#define RTS_DECLSPEC_RESTRICT
#endif

/// @}

/// @defgroup attributes __attribute__ support
/// @brief Portable support for GCC-style @p \__attribute__ annotations
///
/// @code{.cpp}
/// #include "rts/attribute.h"
/// @endcode
///
/// @{

/// @def RTS_HAS_ATTRIBUTE(X)
/// @brief clang's @p __has_attribute
#ifndef __has_attribute
#define RTS_HAS_ATTRIBUTE(X) 0
#else
#define RTS_HAS_ATTRIBUTE(X) __has_attribute(X)
#endif

/// @def RTS_HAS_GCC_ATTRIBUTE(X)
/// @brief modified form of clang's @p __has_attribute
///
/// Always returns 1 on gcc. 
#ifdef GCC
#define RTS_HAS_GCC_ATTRIBUTE(X) 1
#else
#define RTS_HAS_GCC_ATTRIBUTE(X) RTS_HAS_ATTRIBUTE(X)
#endif

/// @def RTS_MALLOC
/// @brief portable version of gcc's @p \__attribute__((malloc))
///
/// Nothing in the returned memory aliases any other pointer
#if RTS_HAS_GCC_ATTRIBUTE(malloc)
#define RTS_MALLOC __attribute__((malloc))
#else
#define RTS_MALLOC
#endif

/// @def RTS_RETURNS_NONNULL
/// @brief portable version of gcc's @p \__attribute__((returns_nonnull))
///
/// Never returns a null pointer
#if RTS_HAS_GCC_ATTRIBUTE(returns_nonnull)
#define RTS_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define RTS_RETURNS_NONNULL
#endif

/// @def RTS_ALLOC_ALIGN
/// @brief portable version of gcc's @p \__attribute_((alloc_align(N)))
/// @param N the number of the argument that specifies the result alignment
///
/// The result will be aligned to at least an N byte boundary
#if RTS_HAS_GCC_ATTRIBUTE(alloc_align)
#define RTS_ALLOC_ALIGN(N) __attribute__((alloc_align(N)))
#else
#define RTS_ALLOC_ALIGN(N)
#endif

/// @def RTS_ALLOC_SIZE
/// @brief portable version of gcc's @p \__attribute__((alloc_size(N)))
/// @param N the number of the argument that specifies the result size
///
/// The result will be exactly N bytes in size
#if RTS_HAS_GCC_ATTRIBUTE(alloc_size)
#define RTS_ALLOC_SIZE(N) __attribute__((alloc_size(N)))
#else
#define RTS_ALLOC_SIZE(N)
#endif

/// @def RTS_CONST
/// @brief portable version of gcc's @p \__attribute__((const))
///
/// Used to annotate a pure function that accesses nothing other than its inputs to compute the output
#if RTS_HAS_GCC_ATTRIBUTE(const)
#define RTS_CONST __attribute__((const))
#else
#define RTS_CONST
#endif

/// @def RTS_PURE
/// @brief portable version of gcc's @p \__attribute__((pure))
///
/// The result is a pure function, and can be subjected to common subexpression elimination
#if RTS_HAS_GCC_ATTRIBUTE(pure)
#define RTS_PURE __attribute__((pure))
#else
#define RTS_PURE
#endif

/// @def RTS_WARN_UNUSED_RESULT
/// @brief portable version of gcc's @p \__attribute__((warn_unused_result))
///
/// Warn if the user doesn't do something with the result of this function.
#if RTS_HAS_GCC_ATTRIBUTE(warn_unused_result)
#define RTS_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define RTS_WARN_UNUSED_RESULT
#endif

/// @def RTS_ALWAYS_INLINE
/// @brief portable version of gcc's @p \__attribute__((always_inline))
///
/// TODO: use __forceinline on other platforms
#if RTS_HAS_GCC_ATTRIBUTE(always_inline)
#define RTS_ALWAYS_INLINE __inline __attribute__((always_inline))
#else
#define RTS_ALWAYS_INLINE __forceinline
#endif


/// @def RTS_UNUSED
/// @brief portable version of gcc's @p \__attribute__((unused))
///
/// Yes, we know this is unused. Shut up, compiler.
#if RTS_HAS_GCC_ATTRIBUTE(unused)
#define RTS_UNUSED __attribute__((unused))
#else
#define RTS_UNUSED
#endif

// c++11 constexpr implies const, if we want to compile in c++11 mode, 
// we'd need to use this to disable constexpr for such functions
#if (defined __cplusplus) && (__cplusplus == 201103L)
#define RTS_MUTABLE_CONSTEXPR
#else
#define RTS_MUTABLE_CONSTEXPR constexpr
#endif

// @}

// @}
