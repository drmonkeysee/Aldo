//
//  bridgeopen.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#ifdef __APPLE__
// NOTE: Swift 6 calling convention for main UI thread platform calls;
// [[...]] syntax not supported for swift_attr.
#define br_pcallc __attribute__((swift_attr("@MainActor")))
#else
#define br_pcallc
#endif

// NOTE: open block for c/c++ bridge headers
#ifdef __cplusplus
#define br_libexport [[gnu::visibility("default")]]
#define br_checkerror [[nodiscard("check error")]]
#define br_ownresult [[nodiscard("take ownership")]]
#define br_noalias
#define br_csz(val)
#define br_nasz(var)
#define br_nacsz(val)
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
#define br_nasz(var) restrict var
#define br_nacsz(val) restrict static val
#define br_nothrow
#define br_empty(T) (T){0}
#endif
