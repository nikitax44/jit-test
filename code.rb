require_relative "macros.rb"

A = 13
B = 14
BUF = 16
SIZE = 17
CHAR_0 = 18
TEN = 19
LIMIT = 20

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

  # R8 - 99
  mov R0, A
  movc R8, 99
  syscall()

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
