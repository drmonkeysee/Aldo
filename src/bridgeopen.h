//
//  bridgeopen.h
//  Aldo
//
//  Created by Brandon Stansbury on 10/14/22.
//

#ifdef __APPLE__
// NOTE: Swift 6 calling convention for main UI thread platform calls;
// [[...]] syntax not supported for swift_attr.
#define aldo_pcallc __attribute__((swift_attr("@MainActor")))
#else
#define aldo_pcallc
#endif

// NOTE: open block for c/c++ bridge headers
#ifdef __cplusplus
#define aldo_export [[gnu::visibility("default")]]
#define aldo_checkerr [[nodiscard("check error")]]
#define aldo_ownresult [[nodiscard("take ownership")]]
#define aldo_noalias
#define aldo_cz(val)
#define aldo_naz(var)
#define aldo_nacz(val)
#define aldo_nothrow noexcept
#define aldo_empty(T) {}
extern "C"
{
#else
#define aldo_export __attribute__((visibility("default")))
#define aldo_checkerr
#define aldo_ownresult
#define aldo_noalias restrict
#define aldo_cz(val) static val
#define aldo_naz(var) restrict var
#define aldo_nacz(val) restrict static val
#define aldo_nothrow
#define aldo_empty(T) (T){0}
#endif
