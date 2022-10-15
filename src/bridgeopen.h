//
//  bridgeopen.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

// NOTE: open block for c/c++ bridge headers
#ifdef __cplusplus
#define aldo_nodiscard(str) [[nodiscard((str))]]
#define aldo_noexcept noexcept
extern "C"
{
#else
#define aldo_nodiscard(str)
#define aldo_noexcept
#endif
