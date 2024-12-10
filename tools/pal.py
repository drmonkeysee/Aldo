import argparse
import itertools
import sys

PAL_LENGTH = 64
CHANNELS = 3
GROUP_SIZE = PAL_LENGTH // 4
PREFIX = '    '
COMMENT_COL = 52


def main():
    parser = argparse.ArgumentParser(
        description=('convert output of palgen_persune into an'
                     ' Aldo C++ default palette code snippet'),
        epilog=('input: python palgen_persune.py'
                ' --skip-plot -cld -phs -5.0 -o default.pal'))
    parser.add_argument('file', type=argparse.FileType('rb'), nargs='?',
                        default=None, metavar='PALETTE FILE')
    args = parser.parse_args()
    if not args.file:
        parser.print_help()
        sys.exit(1)
    _parse_file(args.file.read())


def _parse_file(palbytes):
    if len(palbytes) != PAL_LENGTH * CHANNELS:
        print(f'error: expected {PAL_LENGTH} color'
              f' entries of {CHANNELS}-bytes each')
        sys.exit(2)
    for idx, (r, g, b) in enumerate(itertools.batched(palbytes, CHANNELS)):
        if idx % GROUP_SIZE == 0:
            if idx > 0:
                print()
            print(f'{PREFIX}// {idx:#04x}')
        if (r, g, b) == (0, 0, 0):
            color = 'IM_COL32_BLACK,'
        elif (r, g, b) == (255, 255, 255):
            color = 'IM_COL32_WHITE,'
        else:
            color = f'IM_COL32({r:#x}, {g:#x}, {b:#x}, SDL_ALPHA_OPAQUE),'
        padding = COMMENT_COL - len(color) - len(PREFIX)
        padding = ''.join([' '] * padding)
        comment = f'// #{r:02X}{g:02X}{b:02X}'
        print(f'{PREFIX}{color}{padding}{comment}')


if __name__ == '__main__':
    main()
