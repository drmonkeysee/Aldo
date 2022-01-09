//
//  haltexpr.c
//  Aldo
//
//  Created by Brandon Stansbury on 1/7/22.
//

#include "haltexpr.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

bool haltexpr_parse(const char *str, struct haltexpr *expr, const char **end)
{
    assert(expr != NULL);

    if (str == NULL) return false;

    bool parsed = false;
    char u[2];
    struct haltexpr e;
    for (int i = HLT_NONE + 1; i < HLT_CONDCOUNT; ++i) {
        switch (i) {
#define X(s, pri, scn, ...) \
        case s: parsed = sscanf(str, scn, __VA_ARGS__) == 2; break;
            HALT_EXPR_X
#undef X
        default:
            assert(((void)"INVALID HALT CONDITION", false));
            break;
        }
        if (parsed) {
            *expr = e;
            expr->cond = i;
            break;
        }
    }
    if (end != NULL) {
        const char *const comma = strchr(str, ',');
        *end = comma ? comma + 1 : str + strlen(str);
    }
    return parsed;
}
