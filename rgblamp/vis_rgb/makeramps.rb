def make_hz last
    hz = []
    for i in 0..last
        hz[i] = (i+1).to_f*44100/512
    end
    return hz
end

lastbin = 144
hz = make_hz lastbin
pitch = hz.map{ |x| 69+12*Math.log2(x/440) }
midpoint = (pitch[0] + pitch[lastbin])/2

green = pitch.map{ |x| 1-(midpoint-x).abs/(midpoint-pitch[0]) }

puts "#define RGBM_PIVOTBIN " << green.each_with_index.max[1].to_s
puts "#define RGBM_USEBINS " << (lastbin+1).to_s
puts "static const double green_tab[RGBM_USEBINS] = {"
puts green.join(",\n")
puts "};"
