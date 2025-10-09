require_relative "macros.rb"

A = Reg.new(13)
B = Reg.new(14)
BUF = Reg.new(16)
SIZE = Reg.new(17)
CHAR_0 = Reg.new(18)
TEN = Reg.new(19)
LIMIT = Reg.new(20)

let :setup do
  movc BUF, 128
  movc TEN, 10
  movc CHAR_0, '0'.ord

  movc A, 0
  movc B, 1
  movc LIMIT, 14930352
  jmp :loop
  #
  # nop 100_000_000
end

# forl A, ZERO, C100_000 do
#   inc A
#   slti R0, A, 32000
#   beq R0, ONE, :loop

#   movc A,0

#   mov R0, B
#   movc R8, 99
#   syscall()

#   inc B
#   slti R0, B, 10000
#   beq R0, ONE, :loop
# end


let :loop do
  # nop 100_000_000

  blt LIMIT, A, :exit # check out ble

  call :printA, 4

  # fib step
  add R0, A, B
  mov B, A
  mov A, R0

  jmp :loop
end

let :exit do
  # R8 - 1
  # R0 - exit code
  mov R8, ONE
  movc R0, 0
  syscall()
end

let :printA do
  enter()

  mov R0, A
  movc R8, 99
  syscall()

  ret()
end
