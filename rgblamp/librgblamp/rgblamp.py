# Python interface for library for direct interfacing to the RGB lamp.
# Copyright 2023 Boris Gjenero. Released under the MIT license.
# For more information on functions, see librgblamp.h

from ctypes import *

l = cdll.LoadLibrary('librgblamp.so')

SERCMD_R_RGBOUT = 0
SERCMD_W_RGBOUT = 1
SERCMD_R_RGBVAL = 2
SERCMD_W_RGBVAL = 3
SERCMD_R_POTS = 4
SERCMD_W_DOT = 5
SERCMD_R_DOT = 6

RGB_RSW_MID = 8
RGB_RSW_UP = 2

# Sets up function that takes 3 arguments of the same type and returns bool
def _write3(ctype, func):
    func.argtypes = [ctype, ctype, ctype]
    func.restype = c_bool
    return func

pwm = _write3(c_ushort, l.rgb_pwm)

if hasattr(l, 'rgb_beginasyncpwm'):
    l.rgb_beginasyncpwm.argtypes = []
    l.rgb_beginasyncpwm.restype = c_bool
    beginasyncpwm = l.rgb_beginasyncpwm

if hasattr(l, 'rgb_endasyncpwm'):
    l.rgb_endasyncpwm.argtypes = []
    l.rgb_endasyncpwm.restype = None
    endasyncpwm = l.rgb_endasyncpwm

squared = _write3(c_ushort, l.rgb_squared)
dot = _write3(c_ubyte, l.rgb_dot)
pwm_srgb = _write3(c_double, l.rgb_pwm_srgb)
pwm_srgb256 = _write3(c_ubyte, l.rgb_pwm_srgb256)
matchpwm = _write3(c_ushort, l.rgb_matchpwm)
matchpwm_srgb = _write3(c_double, l.rgb_matchpwm_srgb)
matchpwm_srgb256 = _write3(c_ubyte, l.rgb_matchpwm_srgb256)

l.rgb_fadeprep.argtypes = []
l.rgb_fadeprep.restype = c_bool
fadeprep = l.rgb_fadeprep

# Calls function that fills array of 3 elements of a C type
# Returns that array converted to a list with 3 elements of a Python type
def _get3(libtype, pytype, func):
    crgb = (libtype * 3)()
    if not func(crgb):
        return None
    else:
        return list(map(lambda x: pytype(x), crgb))

# Sets up function that uses _get3() to return results
def _read3(libtype, pytype, func):
    func.argtypes = [POINTER(libtype)]
    func.restype = c_bool
    return lambda : _get3(libtype, pytype, func)

getout = _read3(c_ushort, int, l.rgb_getout)
getout_srgb = _read3(c_double, float, l.rgb_getout_srgb)
getout_srgb256 = _read3(c_ubyte, int, l.rgb_getout_srgb256)

l.rgb_getnormal.argtypes = [c_ubyte, POINTER(c_ushort)]
l.rgb_getnormal.restype = c_bool
def getnormal(sercmd):
    return _get3(c_ushort, int, lambda dest : l.rgb_getnormal(sercmd, dest))

getdot = _read3(c_ushort, int, l.rgb_getdot)

l.rgb_flush.argtypes = []
l.rgb_flush.restype = c_bool
flush = l.rgb_flush

l.rgb_close.argtypes = []
l.rgb_close.restype = None
close = l.rgb_close

l.rgb_open.argtypes = [c_char_p]
l.rgb_open.restype = c_bool
def open(filename):
    return l.rgb_open(filename.encode('utf8'))

# These two "functions" exist as defines in the C header file
def getval():
    return getnormal(SERCMD_R_RGBVAL)

def getpots():
    return getnormal(SERCMD_R_POTS)
