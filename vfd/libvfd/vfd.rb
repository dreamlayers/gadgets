require 'ffi'

module Vfd
  extend FFI::Library
  ffi_lib 'vfd'

  VFDI_PLAY_A = 0
  VFDI_PLAY_B = (1 << 6)
  VFDI_RIGHT_A = 1
  VFDI_RIGHT_B = (1 << 6)
  VFDI_PAUSE_A = 1
  VFDI_PAUSE_B = (1 << 7)
  VFDI_LEFTVU_A = 2
  VFDI_LEFTVU_B = (1 << 4)
  VFDI_RIGHTVU_A = 2
  VFDI_RIGHTVU_B = (1 << 2)
  VFDI_SEC_A = 3
  VFDI_SEC_B = (1 << 3)
  VFDI_MIN_A = 3
  VFDI_MIN_B = (1 << 4)
  VFDI_HR_A = 3
  VFDI_HR_B = (1 << 5)

  VFDTXT_LOOP = 1
  VFDTXT_APPEND = 2
  VFDTXT_MAXCHARS = 46

  VFD_OK = 0
  VFD_FAIL = -1

  # Valid for alarms or byte mode. 0 is also on for byte mode
  VFDPAR_OFF = 0x80
  VFDPAR_ON =  0x40
  VFDPAR_SET = 0xC0

  attach_function :vfd_connect, [ :string ], :int
  attach_function :vfd_disconnect, [ ], :void
  attach_function :vfd_clear, [ ], :int
  attach_function :vfd_full, [ ], :int

  attach_function :vfd_ulbmp, [ :buffer_in ], :int
  attach_function :vfd_dlbmp, [ :buffer_out ], :int

  attach_function :vfd_blitnstr, [ :buffer_inout, :string, :int ], :void
  attach_function :vfd_blitstr, [ :buffer_inout, :string ], :void
  attach_function :vfd_blitvu, [ :buffer_inout, :int, :int ], :void
  attach_function :vfd_blit7segdec, [ :buffer_inout, :int ], :void
  attach_function :vfd_blit7seghex, [ :buffer_inout, :int ], :void

  attach_function :vfd_enterbm, [ ], :int
  attach_function :vfd_exitbm, [ ], :int

  attach_function :vfd_bms7dec, [ :int ], :int
  attach_function :vfd_bms7hex, [ :int ], :int

  attach_function :vfd_bmreadadc, [ :int ], :int
  attach_function :vfd_bmsetvu, [ :int, :int ], :int
  attach_function :vfd_bmind, [ :buffer_in ], :int

  attach_function :vfd_bmclear, [ ], :int

  attach_function :vfd_bmsetscw, [ :int, :int ], :int
  attach_function :vfd_bmntxt, [ :uint, :string, :int ], :int
  attach_function :vfd_bmtxt, [ :uint, :string ], :int
  attach_function :vfd_bmsetc, [ :int, :char ], :int
  attach_function :vfd_bmnsets, [ :int, :string, :int ], :int
  attach_function :vfd_bmsets, [ :int, :string ], :int

  attach_function :vfd_bmparset, [ :int ], :int
  attach_function :vfd_bmparon, [ :int ], :int
  attach_function :vfd_bmparoff, [ :int ], :int
  attach_function :vfd_bmparop, [ :int ], :int
  attach_function :vfd_bmreadpar, [ ], :int

  attach_function :vfd_clearalarms, [ :buffer_out ], :void
  # void vfd_addalarm(unsigned char *c, int op, int par, int h, int m, int s, int r);
  attach_function :vfd_addalarm, [ :buffer_inout, :int, :int, :int, :int, :int, :int ], :void
  attach_function :vfd_setclockto, [ :int, :int, :int, :buffer_in ], :int
  attach_function :vfd_setclock, [ :buffer_in ], :int
end

# This is for testing only
if __FILE__ == $0
  puts Vfd.vfd_connect("/dev/ttyS0")
  puts Vfd.vfd_enterbm
  puts Vfd.vfd_bmtxt(Vfd::VFDTXT_LOOP, "Testing...")
  for i in 0 .. 99
    Vfd.vfd_bms7dec i
  end
  puts Vfd.vfd_bmreadpar
  Vfd.vfd_bmparon 2
  Vfd.vfd_bmtxt(0, "ADC0: " << Vfd.vfd_bmreadadc(0).to_s)
  Vfd.vfd_exitbm
  #Vfd.vfd_setclock(nil)
  Vfd.vfd_disconnect
end
