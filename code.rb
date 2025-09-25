require_relative "macros.rb"

A = 13
B = 14
BUF = 16
SIZE = 17
CHAR_0 = 18
TEN = 19

let :setup do
  movc BUF, 128
  movc TEN, 10
  movc CHAR_0, '0'.ord

  movc A, 0
  movc B, 1
  # jmp :loop
end

let :loop do

  slti R0, A, 32000
  beq R0, ZERO, :exit

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
