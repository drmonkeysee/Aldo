//
//  main.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "ciny.h"

#include <stddef.h>

//
// Test Suites
//

struct ct_testsuite asm_tests(void);
struct ct_testsuite cpu_tests(void);
struct ct_testsuite cpu_absolute_tests(void);
struct ct_testsuite cpu_immediate_tests(void);
struct ct_testsuite cpu_implied_tests(void);
struct ct_testsuite cpu_indirect_tests(void);
struct ct_testsuite cpu_zeropage_tests(void);

static size_t testrunner(int argc, char *argv[argc+1])
{
    const struct ct_testsuite suites[] = {
        asm_tests(),
        cpu_tests(),
        cpu_absolute_tests(),
        cpu_immediate_tests(),
        cpu_implied_tests(),
        cpu_indirect_tests(),
        cpu_zeropage_tests(),
    };
    return ct_run_withargs(suites, argc, argv);
}

//
// Public Interface
//

size_t swift_runner(void)
{
    char *args[] = {"swift-tests", "--ct-colorized=no", NULL};
    return testrunner((sizeof args / sizeof args[0]) - 1, args);
}

int main(int argc, char *argv[argc+1])
{
    const size_t failed = testrunner(argc, argv);
    return failed != 0;
}
