# fixed-meaning registers
ZERO = Reg.new(10)
ONE = Reg.new(11)
FOUR = Reg.new(12)
R0 = Reg.new(0)
R1 = Reg.new(1)
R2 = Reg.new(2)
R8 = Reg.new(8)
SP = Reg.new(9)


def assert(condition, message = "Assertion failed")
  unless condition
    raise AssertionError.new(message)
  end
end

class AssertionError < StandardError; end

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
  ld rd, 0, SP
end

def debug()
  movc R8, 0
  syscall()
end

def blt(rs, rt, label)
  mov rs, rs
  nop 1
  mov rt, rt
  nop 1
  sub R0, rs, rt
  slti R0, R0, 0
  beq R0, ONE, label
end

def call(label, maxbits=32)
  addr = $labels[label]
  movc R8, 100
  if addr !=nil
    movc R0, addr
  else
    $patchup[$pc]=Proc.new {
      addr = $labels[label]
      assert (ones_positions_int32 addr).size <= maxbits, ["addr = ", addr, " did not fit in ", maxbits, " 1-bits: required ", (ones_positions_int32 addr).size].join()
      movc R0, addr
    }
    nop 1+2*maxbits;
  end
  syscall()
end

def enter
  push R0
end

def ret
  pop R0
  movc R8, 100
  syscall()
end

def ble(rs, rt, label)
  sub R0, rs, rt
  slti R0, R0, 1
  beq R0, ONE, label
end

def unique_symbol
  :"sym_#{object_id}"
end

def forl(rd, rs, rt)
  start = unique_symbol
  let start do
    mov rd, rs
    yield
    inc rd
    blt rd, rt, start
  end
end
