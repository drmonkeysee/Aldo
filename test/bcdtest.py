"""Convert bcdtest PRG file into a 32KB ROM file."""
from pathlib import Path


def main():
    folder = Path(__file__).parent
    with open(folder / 'bcdtest.out', 'rb') as f:
        prg = f.read()
    padding = [0xff]
    preamble = padding * 0x600
    rom_file = 'bcdtest.rom'
    with open(folder / rom_file, 'wb') as f:
        f.write(
            bytes(preamble)
            + prg
            + bytes(padding * (2**15 - len(prg) - len(preamble)))
        )
    print('Generated ROM file', rom_file)


if __name__ == '__main__':
    main()
