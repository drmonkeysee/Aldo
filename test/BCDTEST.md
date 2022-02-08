# BCD (Binary-Coded Decimal) Integration Test

The BCD test is derived from Bruce Clark's verification program at [6502.org](http://6502.org). The original listing can be found in [BCD tutorial page (Appendix B)](http://6502.org/tutorials/decimal_mode.html#B) and the header is duplicated here for reference:

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

The source code was compiled using [Easy 6502](https://skilldrick.github.io/easy6502/) with some very minor modifications to match that assembler's syntax expectations, then adjusted to execute within the $86XX memory page rather than Easy 6502's default of $06XX to match the NES memory map. The resulting bytes are saved in **bcdtest.out**.

The out file contains only the bytes consisting of the test program. In order to run it in Aldo, **bcdtest.py** expands the program into a 32KB ROM file, **bcdtest.rom**, with the appropriate offsets and RESET vector so it can be run directly with no additional adjustments. This is all automated with the make target **bcdtest**.

The variable table used by the test ROM is listed below:

```
.define ERROR	$00
.define AR	$01
.define CF	$02
.define DA	$03
.define DNVZC	$04
.define HA	$05
.define HNVZC	$08
.define N1	$09
.define N1H	$0A
.define N1L	$0B
.define N2	$0C
.define N2L	$0D
.define NF	$0E
.define VF	$0F
.define ZF	$10
.define N2H	$11
```

To get a disassembly of the test program run `build/aldo -d test/bcdtest.rom`.
