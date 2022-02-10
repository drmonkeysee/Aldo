# Binary-Coded Decimal (BCD) Integration Test

Included here is an integration test verifying Aldo's binary-coded decimal arithmetic mode. The BCD test is adapted from Bruce Clark's verification program from [6502.org](http://6502.org), the original listing for which can be found at [Decimal Mode tutorial (Appendix B)](http://6502.org/tutorials/decimal_mode.html#B). The program header is duplicated here for reference:

```
; Verify decimal mode behavior
; Written by Bruce Clark.  This code is public domain.
;
; Returns:
;   ERROR = 0 if the test passed
;   ERROR = 1 if the test failed
;
; This routine requires 17 bytes of RAM -- 1 byte each for:
;   AR, CF, DA, DNVZC, ERROR, HA, HNVZC, N1, N1H, N1L, N2, N2L, NF, VF, and ZF
; and 2 bytes for N2H
;
; Variables:
;   N1 and N2 are the two numbers to be added or subtracted
;   N1H, N1L, N2H, and N2L are the upper 4 bits and lower 4 bits of N1 and N2
;   DA and DNVZC are the actual accumulator and flag results in decimal mode
;   HA and HNVZC are the accumulator and flag results when N1 and N2 are
;     added or subtracted using binary arithmetic
;   AR, NF, VF, ZF, and CF are the predicted decimal mode accumulator and
;     flag results, calculated using binary arithmetic
;
; This program takes approximately 1 minute at 1 MHz (a few seconds more on
; a 65C02 than a 6502 or 65816)
;
```

The source code was assembled (after some minor syntax tweaks) using [Easy 6502](https://skilldrick.github.io/easy6502/), then relocated from Easy 6502's default $06XX address space to the $86XX page of the NES memory map. **bcdtest.out** contains the resulting program bytes.

The **bcdtest.py** script expands the program into a 32KB ROM image with the appropriate offsets and RESET vector and stores that in **bcdtest.rom**, which can then be run by Aldo. The make target `bcdtest` automates this conversion step and executes the ROM, printing the result of the integration test.

The test ROM's variable table is listed below:

```
ERROR	= $00
AR	= $01
CF	= $02
DA	= $03
DNVZC	= $04
HA	= $05
HNVZC	= $08
N1	= $09
N1H	= $0A
N1L	= $0B
N2	= $0C
N2L	= $0D
NF	= $0E
VF	= $0F
ZF	= $10
N2H	= $11
```

To get a disassembly of the test program run `build/aldo -d test/bcdtest.rom`.
