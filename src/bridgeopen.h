//
//  bridgeopen.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

// NOTE: open block for c/c++ bridge headers
#ifdef __cplusplus
#define br_checkreturn(str) [[nodiscard((str))]]
#define br_noalias
#define br_noalias_sz(var)
#define br_noalias_csz(val)
#define br_nothrow noexcept
extern "C"
{
#else
#define br_checkreturn(str)
#define br_noalias restrict
#define br_noalias_sz(var) restrict var
#define br_noalias_csz(val) restrict static val
#define br_nothrow
#endif
