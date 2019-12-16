#!/usr/bin/env python3

REMOTE = 'DC550D_H300'

PREFIX = '/usr/local/bin/canim in .5 '

COLORS = [
    ( 'coloranim_red', 'KEY_RED', [1, -1, -1] ),
    ( 'coloranim_green', 'KEY_GREEN', [-1, 1, -1] ),
    ( 'coloranim_blue', 'KEY_BLUE', [-1, -1, 1] ),
    # [1, 1, 1] is too blue
    ( 'coloranim_white', 'KEY_YELLOW', [1, .94, .85] )
]

INTENSITIES = list(map(lambda x: 'KEY_' + str(x), range(0,10)))

KEY = '''    begin
        prog   = irexec
        remote = %s
        button = %%s
        config = %s%%s
    end''' % (REMOTE, PREFIX)

MODE = '''begin
    prog   = irexec
    remote = %s
    button = %%s
    mode = %%s
end''' % REMOTE

def canim_float(f):
    s = '%.5g' % f
    return '0' if s == '0' else s.lstrip('0')

def canim_str(template, brightness):
    return ' '.join(map(lambda x: '-' if x < 0 else canim_float(x * brightness),
                        template))

def canim_keys(template, keys, color):
    print('begin ' + color[0])
    scale = len(keys) - 1
    for idx, key in enumerate(keys):
        print(template % (key, canim_str(color[2], idx / scale)))
    print('end ' + color[0])

for c in COLORS:
    canim_keys(KEY, INTENSITIES, c)

for c in COLORS:
    print(MODE % (c[1], c[0]))
