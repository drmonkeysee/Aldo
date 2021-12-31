//
//  batch.c
//  Aldo
//
//  Created by Brandon Stansbury on 12/30/21.
//

#include "control.h"
#include "snapshot.h"
#include "ui.h"

#include <assert.h>

//
// UI Interface Implementation
//

static void batch_init(void)
{
    // TOOD: figure out clock for batch mode
    assert(((void)"UNIMPLEMENTED", false));
}

static void batch_cleanup(void)
{
    // NOTE: nothing to do
}

static void batch_tick_start(struct control *appstate,
                             const struct console_state *snapshot)
{
    assert(((void)"UNIMPLEMENTED", false));
}

static void batch_tick_end(void)
{
    assert(((void)"UNIMPLEMENTED", false));
}

static int batch_pollinput(void)
{
    return 0;
}

static void batch_refresh(const struct control *appstate,
                          const struct console_state *snapshot)
{
    assert(((void)"UNIMPLEMENTED", false));
}

//
// Public Interface
//

struct ui_interface ui_batch_init(void)
{
    return (struct ui_interface){
        batch_tick_start,
        batch_tick_end,
        batch_pollinput,
        batch_refresh,
        batch_cleanup,
    };
}
