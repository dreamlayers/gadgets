binavg = Array.new
File.readlines(ARGV[0]).each { |x|
    line = x.strip.split(":")
    binavg[line[0].to_i] = line[1].to_f
}

firstbin = 0
while binavg[firstbin] == nil do
    firstbin += 1
end
lastbin = binavg.length - 1

binttl = 0
for i in firstbin..lastbin do
    binttl += binavg[i]
end

target = binttl/3.0

print firstbin.to_s << ".."
sum = 0
for i in firstbin..lastbin do
    lastsum = sum
    sum += binavg[i]
    if (sum > target) then
        err = (sum - target).abs
        alterr = (lastsum - target).abs
        if (alterr < err) then
            cut = i-1
            sum = binavg[i]
        else
            cut = i
            sum = 0
        end
        puts cut
        print (cut+1).to_s << ".."
    end
end
puts lastbin.to_s
