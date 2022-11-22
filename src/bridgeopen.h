//
//  bridgeopen.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

// NOTE: open block for c/c++ bridge headers
#ifdef __cplusplus
#define br_libexport [[gnu::visibility("default")]]
#define br_checkerror [[nodiscard("check error")]]
#define br_ownresult [[nodiscard("take ownership of result")]]
#define br_noalias
#define br_csz(val)
#define br_noalias_sz(var)
#define br_noalias_csz(val)
#define br_nothrow noexcept
#define br_empty(T) {}
extern "C"
{
#else
#define br_libexport __attribute__((visibility("default")))
#define br_checkerror
#define br_ownresult
#define br_noalias restrict
#define br_csz(val) static val
#define br_noalias_sz(var) restrict var
#define br_noalias_csz(val) restrict static val
#define br_nothrow
#define br_empty(T) (T){0}
#endif
