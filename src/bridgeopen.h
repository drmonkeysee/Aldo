//
//  bridgeopen.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

// NOTE: open block for c/c++ bridge headers
#ifdef __cplusplus
#define bd_checkreturn(str) [[nodiscard((str))]]
#define bd_noalias
#define bd_noalias_sz(var)
#define bd_noalias_fxdsz(val)
#define bd_nothrow noexcept
extern "C"
{
#else
#define bd_checkreturn(str)
#define bd_noalias restrict
#define bd_noalias_sz(var) restrict var
#define bd_noalias_fxdsz(val) restrict static val
#define bd_nothrow
#endif
