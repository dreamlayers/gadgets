#!/usr/bin/env python3

from pathlib import Path

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

KEYSINMODE = list(map(lambda x: 'KEY_' + str(x), range(0,10)))

CANIM_TEMPLATE = '/usr/local/bin/canim in .5 %s'

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
      m3u_list('/var/lib/mopidy/playlists/Radio.m3u8', 10)),
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
