//
//  main.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "ciny.h"

#include <stddef.h>

//
// MARK: - Test Suites
//

void
    setup_testbus(),
    teardown_testbus();

struct ct_testsuite argparse_tests(),
                    bus_tests(),
                    bytes_tests(),
                    cpu_tests(),
                    cpu_absolute_tests(),
                    cpu_branch_tests(),
                    cpu_immediate_tests(),
                    cpu_implied_tests(),
                    cpu_indirect_tests(),
                    cpu_interrupt_handler_tests(),
                    cpu_interrupt_signal_tests(),
                    cpu_jump_tests(),
                    cpu_peek_tests(),
                    cpu_stack_tests(),
                    cpu_subroutine_tests(),
                    cpu_zeropage_tests(),
                    debug_tests(),
                    dis_tests(),
                    dis_peek_tests(),
                    haltexpr_tests(),
                    ppu_tests(),
                    ppu_register_tests(),
                    ppu_render_tests();

static size_t testrunner(int argc, char *argv[argc+1])
{
    struct ct_testsuite suites[] = {
        argparse_tests(),
        bus_tests(),
        bytes_tests(),
        cpu_tests(),
        cpu_absolute_tests(),
        cpu_branch_tests(),
        cpu_immediate_tests(),
        cpu_implied_tests(),
        cpu_indirect_tests(),
        cpu_interrupt_handler_tests(),
        cpu_interrupt_signal_tests(),
        cpu_jump_tests(),
        cpu_peek_tests(),
        cpu_stack_tests(),
        cpu_subroutine_tests(),
        cpu_zeropage_tests(),
        debug_tests(),
        dis_tests(),
        dis_peek_tests(),
        haltexpr_tests(),
        ppu_tests(),
        ppu_register_tests(),
        ppu_render_tests(),
    };
    setup_testbus();
    auto result = ct_run_withargs(suites, argc, argv);
    teardown_testbus();
    return result;
}

//
// MARK: - Public Interface
//

size_t swift_runner()
{
    char *args[] = {"swift-tests", "--ct-colorized=no", nullptr};
    return testrunner((sizeof args / sizeof args[0]) - 1, args);
}

int main(int argc, char *argv[argc+1])
{
    auto failed = testrunner(argc, argv);
    return failed != 0;
}
