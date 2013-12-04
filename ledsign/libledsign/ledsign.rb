require 'ffi'

module LEDSign
  extend FFI::Library
  ffi_lib 'ledsign'

  attach_function :open, :sign_open, [ :string ], :int
  attach_function :close, :sign_close, [ ], :void

  attach_function :clear, :sign_clear, [ ], :int
  attach_function :full, :sign_full, [ ], :int

  SIGN_APPEND = 1
  SIGN_LOOP = 2
  attach_function :nhwprint, :sign_nhwprint, [ :string, :size_t, :uint ], :int
# attach_function :hwprint, :sign_hwprint, [ :string, int hwpflags ], :int

  class Sign_font < FFI::Struct
    layout  :data, [ :uchar, 256*8 ],
            :height, :uint,
            :width, :uint
  end
  attach_function :loadfont, :sign_loadfont, [ :string, Sign_font.by_ref ], :int
# attach_function :fontfromresource, :sign_fontfromresource, [ WORD resid, Sign_font.by_ref ], :int

  attach_function :nswprint, :sign_nswprint, [ :string, :size_t,
                                               Sign_font.by_ref, :int,
                                               :int ], :int
  attach_function :swprint, :sign_swprint, [ :string,
                                             Sign_font.by_ref, :int,
                                             :int ], :int

  SIGN_FONT_INVERTED = 1
  SIGN_FONT_BOLD = 2
  SIGN_FONT_ITALIC = 4
  attach_function :scrl_start, :sign_scrl_start, [ ], :int
  attach_function :scrl_char, :sign_scrl_char,
                  [ :char, Sign_font.by_ref, :int, :int ], :int
  attach_function :scrl_str, :sign_scrl_str,
                  [ :string, Sign_font.by_ref, :int ], :int
  attach_function :scrl_nstr, :sign_scrl_nstr,
                  [ :string, :size_t, Sign_font.by_ref, :int ], :int

  attach_function :scru_str, :sign_scru_str,
                  [ :string, Sign_font.by_ref, :int ], :int
  attach_function :scru_nstr, :sign_scru_nstr,
                  [ :string, :size_t, Sign_font.by_ref, :int ], :int

  attach_function :ul_str, :sign_ul_str,
                  [ :string, Sign_font.by_ref, :int ], :int
  attach_function :ul_nstr, :sign_ul_nstr,
                  [ :string, :size_t, Sign_font.by_ref, :int ], :int

  attach_function :ul_bmap, :sign_ul_bmap, [ :buffer_in ], :int
  attach_function :dl_bmap, :sign_dl_bmap, [ :buffer_out ], :int
# attach_function :dl_represent, :sign_dl_represent, [ FILE *f ], :int
  attach_function :setabortpoll, :sign_setabortpoll, [ :pointer ], :void
end

# This is for testing only
if __FILE__ == $0
end
