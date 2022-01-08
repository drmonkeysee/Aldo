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

const enum haltcondition FirstCondition = HALT_NONE + 1;

//
// Public Interface
//

bool haltexpr_parse(const char *str, struct haltexpr *expr, const char **end)
{
    bool parsed = false;
    char unit[2];
    for (int i = FirstCondition; i < HLT_CONDCOUNT; ++i) {
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
            if (end != NULL) {
                char *const comma = strchr(str, ',');
                *end = comma ? comma + 1 : str + strlen(str) + 1;
            }
            break;
        }
    }
    return parsed;
}
