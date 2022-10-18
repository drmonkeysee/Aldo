//
//  bridgeopen.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

// NOTE: open block for c/c++ bridge headers
#ifdef __cplusplus
#define aldo_checkreturn(str) [[nodiscard((str))]]
#define aldo_nothrow noexcept
extern "C"
{
#else
#define aldo_checkreturn(str)
#define aldo_nothrow
#endif
