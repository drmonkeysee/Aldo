"""Convert bcdtest PRG file into a 32KB ROM file."""
from pathlib import Path

BANK_SIZE = 2**15
FILE_NAME = 'bcdtest'
PADDING = [0xff]
RESET_VECTOR = 0xfffc - 0x8000
START_OFFSET = 0x600


def main():
    folder = Path(__file__).parent
    with open(folder / f'{FILE_NAME}.out', 'rb') as f:
        prg = f.read()

    preamble = bytes(PADDING * START_OFFSET)
    bank_fill = bytes(PADDING * (BANK_SIZE - len(prg) - len(preamble)))
    prg_bank = bytearray(preamble + prg + bank_fill)
    prg_bank[RESET_VECTOR:RESET_VECTOR + 2] = [0x0, 0x86]

    rom_file = f'{FILE_NAME}.rom'
    with open(folder / rom_file, 'wb') as f:
        count = f.write(prg_bank)
    assert count == BANK_SIZE
    print('Generated ROM file', rom_file)


if __name__ == '__main__':
    main()
