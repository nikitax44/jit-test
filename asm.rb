$out = File.open(ARGV[0], "wb+")
$pc = 0
$labels = {}
$patchup = {}
$verbose = false

def emit(opcode, payload)
  # if $verbose
  #   puts ['^ ', $pc].join('')
  # end
  bits = (opcode << 26) | (payload&0x3ffffff)
  $out.write([bits].pack("V"))
  $pc+=4
end

def jmp(label)
  if $verbose
    puts ["jmp ", label].join()
  end

  addr = $labels[label]
  if addr !=nil
    emit(0b010110, addr>>2)
  else
    $patchup[$pc]=Proc.new {jmp label}
    emit(0, -1)
  end
end

def usat(rd, rs, n) # clamp to n bits
  if $verbose
    puts ["usat r", rd, ", r", rs, ", ", n].join('')
  end

  emit(0b100010, (rd<< 21) | (rs << 16) | (n << 11))
end

def beq(rs, rt, label)
  if $verbose
    puts ["beq r", rs, ", r", rt, ", ", label].join('')
  end

  addr = $labels[label]
  if addr != nil
    diff = (addr-$pc)>>2
    emit(0b010011, (rs << 21) | (rt << 16) | (diff & 0xffff))
  else
    $patchup[$pc]=Proc.new {beq rs, rt, label}
    emit(0, -1)
  end
end

def slti(rt, rs, imm)
  if $verbose
    puts ["slti r", rt, ", r", rs, ", ", imm].join('')
  end

  emit(0b111011, (rs<< 21) | (rt << 16) | (imm & 0xffff))
end

def movz(rd, rs, rt)
  if $verbose
    puts ["movz r", rd, ", r", rs, ", r", rt].join('')
  end

  emit(0, (rs<< 21) | (rt << 16) | (rd << 11) | (0b100))
end

def stp(rt1, rt2, base, offset)
  if $verbose
    puts ["stp r", rt1, ", r", rt2, ", r", base, '[', offset, ']'].join('')
  end

  emit(0b111001, (base << 21) | (rt1 << 16) | (rt2 << 11) | (offset & 0x7ff))
end

def selc(rd, rs1, rs2)
  if $verbose
    puts ["selc r", rd, ", r", rs1, ", r", rs2].join('')
  end

  emit(0, (rd << 21) | (rs1 << 16) | (rs2 << 11) | (0b11))
end

def syscall()
  if $verbose
    puts "syscall"
  end

  emit(0, 0b011001)
end

def sub(rd, rs, rt)
  if $verbose
    puts ["sub r", rd, ", r", rs, ", r", rt].join('')
  end

  emit(0, (rs << 21) | (rt << 16) | (rd << 11) | (0b011111))
end

def add(rd, rs, rt)
  if $verbose
    puts ["add r", rd, ", r", rs, ", r", rt].join('')
  end

  emit(0, (rs << 21) | (rt << 16) | (rd << 11) | (0b011010))
end

def rbit(rd, rs)
  if $verbose
    puts ["rbit r", rd, ", r", rs].join('')
  end

  emit(0, (rd << 21) | (rs << 16) | (0b011101))
end

def ld(rt, offset, base)
  if $verbose
    puts ["ld r", rt, ", r", base, '[', offset, ']'].join('')
  end

  emit(0b100011, (base << 21) | (rt << 16) | (offset & 0xffff))
end

def rori(rd, rs, imm5)
  if $verbose
    puts ["rori r",rd, ", r", rs, ", ", imm5 & 31].join('')
  end
  emit(0b001100, (rd << 21) | (rs << 16) | ((imm5 & 31) << 11))
end

def st(rt, offset, base)
  if $verbose
    puts ["st r", rt, ", r", base, '[', offset, ']'].join('')
  end

  emit(0b100101, (base << 21) | (rt << 16) | (offset & 0xffff))
end

def ud2()
  if $verbose
    puts "ud2"
  end
  emit(0,0)
end

def int3()
  if $verbose
    puts "int3"
  end
  emit(0,1)
end

def nop(n=1)
  if $verbose
    puts ["nop ", n].join('')
  end
  bytes = [2].pack("V")
  bytes1024 = bytes*1024
  for i in 1..(n/1024) do
    $out.write(bytes1024)
  end
  for i in 1..(n%1024) do
    $out.write(bytes)
  end
  $pc+=4*n
end

def let(name, &block)
  if $verbose
    puts [name, ":"].join('')
  end
  $labels[name]=$pc
  if block!=nil
    block.call()
  end
  if $verbose
    puts()
  end
end

require_relative "code.rb"

$verbose  = false

$patchup.map {|pc, block|
  $pc=pc
  $out.seek(pc, IO::SEEK_SET)
  block.call
}
$out.close()
