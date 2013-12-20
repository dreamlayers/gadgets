# Ruby bindings for RGB lamp direct interfacing library
# Copyright 2013 Boris Gjenero. Released under the MIT license.

require 'ffi'

module RGBLamp
  extend FFI::Library
  ffi_lib 'rgblamp'
  # ** Commands **

  # These are typically needed because they are encapsulated
  # by functions below

  SERCMD_R_RGBOUT = 0
  SERCMD_W_RGBOUT = 1
  SERCMD_R_RGBVAL = 2
  SERCMD_W_RGBVAL = 3
  SERCMD_R_POTS = 4
  SERCMD_W_DOT = 5
  SERCMD_R_DOT = 6

  # Switch bits for right switch in PWM values

  RGB_RSW_MID = 8
  RGB_RSW_UP = 2

  # ** Writing functions **

  # Send raw PWM values (0-4095)
  attach_function :pwm, :rgb_pwm, [ :ushort, :ushort, :ushort ], :bool

  # Start a thread which sends PWM values, so rgb_pwm returns immediately
  #attach_function :beginasyncpwm, :rgb_beginasyncpwm, [ ], :bool

  # Stop PWM value sending thread, so rgb_pwm returns immediately
  #attach_function :endasyncpwm, :rgb_endasyncpwm, [ ], :void

  #Send 16 bit 0 to 0xFFFF values corresponding to 0 to 1.0.
  # They are squared before being converted to PWM values.
  attach_function :squared, :rgb_squared, [ :ushort, :ushort, :ushort ], :bool

  # Set dot correction values (0-63)
  attach_function :dot, :rgb_dot, [ :uchar, :uchar, :uchar ], :bool

  # Send 0 to 1.0 floating point sRGB values, which are converted to
  # raw PWM values before being sent.
  attach_function :pwm_srgb, :rgb_pwm_srgb,
                  [ :double, :double, :double ], :bool

  # Send 0 to 255 sRGB values, which are converted to
  # raw PWM values before being sent.
  attach_function :pwm_srgb256, :rgb_pwm_srgb256,
                  [ :uchar, :uchar, :uchar ], :bool

  # Set raw PWM values via rgb_squared, so fading works after
  attach_function :matchpwm, :rgb_matchpwm,
                  [ :ushort, :ushort, :ushort ], :bool

  # Same as rgb_matchpwm, taking 0 to 1.0 sRGB input
  attach_function :matchpwm_srgb, :rgb_matchpwm_srgb,
                  [ :double, :double, :double ], :bool

  # Use rgbout to re-set PWM values via rgb_squared, so fading works after
  attach_function :fadeprep, :rgb_fadeprep, [ ], :bool


  # ** Reading functions **

  # Get raw values that were output to TLC5940
  # These are raw PWM values unless the last function was dot correction.
  # Array of 3 unsighed short
  attach_function :rgb_getout, [ :buffer_out ], :bool
  def RGBLamp.getout
    ptr = FFI::MemoryPointer.new :ushort, 3
    rgb_getout ptr
    ptr.read_array_of_ushort(3)
  end

  # Same as rgb_getout, but returning 0.0 to 1.0 sRGB value
  # Array of 3 double
  attach_function :rgb_getout_srgb, [ :buffer_out ], :bool
  def RGBLamp.getout_srgb
    ptr = FFI::MemoryPointer.new :double, 3
    rgb_getout_srgb ptr
    ptr.read_array_of_double(3)
  end

  # Send command, get 3 values. Use defines below
  # Array of 3 unsigned short
  attach_function :rgb_getnormal, [ :uchar, :pointer ], :bool

  # Get not gamma corrected values (3 values, 0-0xFFFF)
  def RGBLamp.getpots
    ptr = FFI::MemoryPointer.new :ushort, 3
    rgb_getnormal SERCMD_R_RGBVAL, ptr
    ptr.read_array_of_ushort(3)
  end

  # Get pots and switches
  def RGBLamp.getpots
    ptr = FFI::MemoryPointer.new :ushort, 3
    rgb_getnormal SERCMD_R_POTS, ptr
    ptr.read_array_of_ushort(3)
  end

  # Get dot correction values
  # Array of 3 unsigned short
  attach_function :rgb_getdot, [ :buffer_out ], :bool
  def RGBLamp.getdot
    ptr = FFI::MemoryPointer.new :ushort, 3
    rgb_getdot ptr
    ptr.read_array_of_ushort(3)
  end

  attach_function :flush, :rgb_flush, [ ], :bool

  # ** Initialization functions **

  attach_function :close, :rgb_close, [ ], :void

  attach_function :open, :rgb_open, [ :string ], :bool
end

# This is for testing only
if __FILE__ == $0
  RGBLamp.open "/dev/ttyUSB0"
  for i in 0..100
    RGBLamp.pwm i, i, i/2
  end
#  while true
    puts RGBLamp.getpots.to_s
#  end
  puts RGBLamp.getdot.to_s
  RGBLamp.close
end
