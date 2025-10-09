$out = File.open(ARGV[0], "wb+")
$pc = 0
$labels = {}
$patchup = {}
if $out.tty?
  $verbose = true
  $out = nil
end

class Reg
  attr_accessor :id
  def initialize(id)
    @id = id
  end

  def to_s
    "X[#{@id}]".rjust(5)
  end
end

def emit(opcode, payload)
  # if $verbose
  #   puts ['^ ', $pc].join('')
  # end
  bits = (opcode << 26) | (payload&0x3ffffff)
  $out&.write([bits].pack("V"))
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
    puts ["usat ", rd, ", ", rs, ", ", n].join('')
  end

  emit(0b100010, (rd.id << 21) | (rs.id << 16) | (n << 11))
end

def beq(rs, rt, label)
  if $verbose
    puts ["beq  ", rs, ", ", rt, ", ", label].join('')
  end

  addr = $labels[label]
  if addr != nil
    diff = (addr-$pc)>>2
    emit(0b010011, (rs.id << 21) | (rt.id << 16) | (diff & 0xffff))
  else
    $patchup[$pc]=Proc.new {beq rs, rt, label}
    emit(0, -1)
  end
end

def slti(rt, rs, imm)
  if $verbose
    puts ["slti ", rt, ", ", rs, ", ", imm].join('')
  end

  emit(0b111011, (rs.id << 21) | (rt.id << 16) | (imm & 0xffff))
end

def movz(rd, rs, rt)
  if $verbose
    puts ["movz ", rd, ", ", rs, ", ", rt].join('')
  end

  emit(0, (rs.id << 21) | (rt.id << 16) | (rd.id << 11) | (0b100))
end

def stp(rt1, rt2, base, offset)
  if $verbose
    puts ["stp  ", rt1, ", ", rt2, ", ", base, '+', offset].join('')
  end

  emit(0b111001, (base.id << 21) | (rt1.id << 16) | (rt2.id << 11) | (offset & 0x7ff))
end

def selc(rd, rs1, rs2)
  if $verbose
    puts ["selc ", rd, ", ", rs1, ", ", rs2].join('')
  end

  emit(0, (rd.id << 21) | (rs1.id << 16) | (rs2.id << 11) | (0b11))
end

def syscall()
  if $verbose
    puts "syscall"
  end

  emit(0, 0b011001)
end

def sub(rd, rs, rt)
  if $verbose
    puts ["sub  ", rd, ", ", rs, ", ", rt].join('')
  end

  emit(0, (rs.id << 21) | (rt.id << 16) | (rd.id << 11) | (0b011111))
end

def add(rd, rs, rt)
  if $verbose
    puts ["add  ", rd, ", ", rs, ", ", rt].join('')
  end

  emit(0, (rs.id << 21) | (rt.id << 16) | (rd.id << 11) | (0b011010))
end

def rbit(rd, rs)
  if $verbose
    puts ["rbit ", rd, ", ", rs].join('')
  end

  emit(0, (rd.id << 21) | (rs.id << 16) | (0b011101))
end

def ld(rt, offset, base)
  if $verbose
    puts ["ld   ", rt, ", ", base, '+', offset].join('')
  end

  emit(0b100011, (base.id << 21) | (rt.id << 16) | (offset & 0xffff))
end

def rori(rd, rs, imm5)
  if $verbose
    puts ["rori ",rd, ", ", rs, ", ", imm5 & 31].join('')
  end
  emit(0b001100, (rd.id << 21) | (rs.id << 16) | ((imm5 & 31) << 11))
end

def st(rt, offset, base)
  if $verbose
    puts ["st   ", rt, ", ", base, '+', offset].join('')
  end

  emit(0b100101, (base.id << 21) | (rt.id << 16) | (offset & 0xffff))
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
    $out&.write(bytes1024)
  end
  for i in 1..(n%1024) do
    $out&.write(bytes)
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
  $out&.seek(pc, IO::SEEK_SET)
  block.call
}
$out&.close()
