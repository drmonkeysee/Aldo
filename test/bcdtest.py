"""Convert bcdtest PRG file into a 32KB ROM file."""
from pathlib import Path

RESET_VECTOR = 0xfffc - 0x8000
START_OFFSET = 0x600


def main():
    folder = Path(__file__).parent
    with open(folder / 'bcdtest.out', 'rb') as f:
        prg = f.read()
    padding = [0xff]
    preamble = bytes(padding * START_OFFSET)
    bank_fill = bytes(padding * (2**15 - len(prg) - len(preamble)))
    rom_file = 'bcdtest.rom'
    prg_bank = bytearray(preamble + prg + bank_fill)
    prg_bank[RESET_VECTOR:RESET_VECTOR + 2] = [0x0, 0x86]
    with open(folder / rom_file, 'wb') as f:
        count = f.write(prg_bank)
    assert count == 2**15
    print('Generated ROM file', rom_file)


if __name__ == '__main__':
    main()
