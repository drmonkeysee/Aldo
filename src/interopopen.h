//
//  interopopen.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

// NOTE: open block for c/c++ interop headers
#ifdef __cplusplus
#define ALDO_NODISCARD(str) [[nodiscard((str))]]
#define ALDO_NOEXCEPT noexcept
extern "C"
{
#else
#define ALDO_NODISCARD(str)
#define ALDO_NOEXCEPT
#endif
