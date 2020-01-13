#!/usr/bin/env python3

from pathlib import Path

# Main remote

REMOTE = 'DC550D_H300'

KEY = '''    begin
        prog   = irexec
        remote = %s
        button = %%s
        config = %%s
    end''' % REMOTE

MODE = '''begin
    prog   = irexec
    remote = %s
    button = %%s
    mode = %%s
end''' % REMOTE

def key_range(prefix, r):
    return list(map(lambda x: prefix + str(x), r))

KEYSINMODE = key_range('KEY_', range(0,10))

CANIM_TEMPLATE = '/usr/local/bin/canim in .5 %s'

# RGB bulb remote

BULB_KEY = '''begin
    prog   = irexec
    remote = RGBbulb
    button = %s
    config = %s
end'''

BULB_KEYS = [ 'KEY_RED', 'KEY_GREEN', 'KEY_BLUE', 'KEY_BRIGHTNESS_MAX' ] + \
    key_range('KEY_F', range(1,13))

BULB_COLORS = [
    '1 0 0',
    '0 1 0',
    '0 0 1',
    '1 .75 .35',
    '1 .35 0',
    '0 1 .6',
    '.12 .18 1',
    '1 .6 0',
    '0 1 .8',
    '.9 0 1',
    '1 .72 0',
    '0 1 1',
    '1 0 1',
    '1 .85 0',
    '.20 .45 1',
    '1 0 .85'
]

def canim_float(f):
    s = '%.5g' % f
    return '0' if s == '0' else s.lstrip('0')

def canim_str(template, brightness):
    return ' '.join(map(lambda x: '-' if x < 0 else \
                        canim_float(x * brightness / (len(KEYSINMODE) - 1)),
                        template))

MOPIDY_TEMPLATE = 'mpc stop; mpc clear; mpc add "%s"; mpc play'

def m3u_list(filename, num):
    lines = []
    for l in open(filename):
        if l.startswith('#'):
            continue
        else:
            lines.append(l.rstrip())
            if len(lines) == num:
                break
    return lines

def dir_list(directory, glob, num):
    dirs = []
    pathlist = Path(directory).glob(glob)
    for path in pathlist:
        if path.is_dir():
            dirs.append('Files/' + path.name.replace('"', '\\"'))
            if len(dirs) == num:
                break
    dirs.sort()
    return dirs

MODES = [
    ( 'coloranim_red', 'KEY_RED',
       CANIM_TEMPLATE, lambda x: canim_str([1, -1, -1], x) ),
    ( 'coloranim_green', 'KEY_GREEN',
       CANIM_TEMPLATE, lambda x: canim_str([-1, 1, -1], x) ),
    ( 'coloranim_blue', 'KEY_BLUE',
       CANIM_TEMPLATE, lambda x: canim_str([-1, -1, 1], x) ),
    # [1, 1, 1] is too blue
    ( 'coloranim_white', 'KEY_YELLOW',
       CANIM_TEMPLATE, lambda x: canim_str([1, .94, .85], x) ),
    ( 'mopidy_radio', 'KEY_CLEAR', MOPIDY_TEMPLATE,
      m3u_list('/var/lib/mopidy/m3u/Radio.m3u8', 10)),
    ( 'mopidy_dir', 'KEY_ENTER', MOPIDY_TEMPLATE,
      dir_list('/home/kodi/Music/', '*-*', 10))
]

for mode in MODES:
    print('begin ' + mode[0])
    if callable(mode[3]):
        for idx, key in enumerate(KEYSINMODE):
            print(KEY % (key, mode[2] % mode[3](idx)))
    else:
        for idx, s in enumerate(mode[3]):
            print(KEY % (KEYSINMODE[idx], mode[2] % s))
    print('end ' + mode[0])
    print(MODE % (mode[1], mode[0]))

for k, c in zip(BULB_KEYS, BULB_COLORS):
    print(BULB_KEY % (k, CANIM_TEMPLATE % c))
