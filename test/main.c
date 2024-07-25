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
    setup_testbus(void),
    teardown_testbus(void);

struct ct_testsuite argparse_tests(void),
                    bus_tests(void),
                    bytes_tests(void),
                    cpu_tests(void),
                    cpu_absolute_tests(void),
                    cpu_branch_tests(void),
                    cpu_immediate_tests(void),
                    cpu_implied_tests(void),
                    cpu_indirect_tests(void),
                    cpu_interrupt_handler_tests(void),
                    cpu_interrupt_signal_tests(void),
                    cpu_jump_tests(void),
                    cpu_peek_tests(void),
                    cpu_stack_tests(void),
                    cpu_subroutine_tests(void),
                    cpu_zeropage_tests(void),
                    debug_tests(void),
                    dis_tests(void),
                    dis_peek_tests(void),
                    haltexpr_tests(void),
                    ppu_tests(void),
                    ppu_register_tests(void);

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
    };
    setup_testbus();
    size_t result = ct_run_withargs(suites, argc, argv);
    teardown_testbus();
    return result;
}

//
// MARK: - Public Interface
//

size_t swift_runner(void)
{
    char *args[] = {"swift-tests", "--ct-colorized=no", NULL};
    return testrunner((sizeof args / sizeof args[0]) - 1, args);
}

int main(int argc, char *argv[argc+1])
{
    size_t failed = testrunner(argc, argv);
    return failed != 0;
}
