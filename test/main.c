//
//  main.c
//  Aldo-Tests
//
//  Created by Brandon Stansbury on 2/24/21.
//

#include "tests.h"

int main(int argc, char *argv[argc+1])
{
    const struct ct_testsuite suites[] = {
        asm_tests(),
    };
    const size_t results = ct_run_withargs(suites, argc, argv);
    return results != 0;
}
