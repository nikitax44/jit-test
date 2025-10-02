# fixed-meaning registers
ZERO = 10
ONE = 11
FOUR = 12
R0 = 0
R1 = 1
R2 = 2
R8 = 8
SP = 9



let :setup1 do
  sub ZERO, ZERO, ZERO
  slti ONE, ZERO, 1
end

def mov(rd, rs)
  rori rd, rs, 0
end

def inc(rd)
  add rd, rd, ONE
end

def ones_positions_int32(n)
  n &= 0xFFFFFFFF
  positions = []
  32.times do |i|
    positions << i if (n & (1 << i)) != 0
  end
  positions
end

def roli(rd, rs, imm5)
  rori rd, rs, 32-imm5
end

def movc(rd, val32)
  ones = ones_positions_int32 val32

  if ones.size == 0
    mov rd, ZERO
    return
  end

  if ones.size == 1
    roli rd, ONE, ones[0]
    return
  end

  ones.unshift(0) # prepend 0

  roli rd, ONE, ones[-1]-ones[-2]
  # now re're pointing at the -2'th should-be-one

  for i in 2...(ones.length)
    inc rd

    roli rd, rd, ones[-i] - ones[-i-1] unless ones[-i]==ones[-i-1]
  end
end

let :setup2 do
  movc FOUR, 4
  movc SP, 1024
end

def push(rs)
  st rs, 0, SP
  add SP, SP, FOUR
end

def pop(rd)
  sub SP, SP, FOUR
  ld rs, 0, SP
end

def debug()
  movc R8, 0
  syscall()
end
