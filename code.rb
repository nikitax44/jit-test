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

  movc A, 1
  movc B, 1
  # jmp :loop
end

let :loop do

  slti R0, A, 100
  beq R0, ZERO, :exit

  # fib step
  add R0, A, B
  mov B, A
  mov A, R0

  # jmp :exit
  jmp :a_to_dec_and_write
end

let :a_to_dec_and_write do
  st CHAR_0, 0, BUF
  movc SIZE, 1

  jmp :write_buf_size
end

let :write_buf_size do
  # R8 - 4
  # R0 - fd
  # R1 - buf
  # R2 - count
  rori R8, ONE, 30
  mov R0, ONE
  mov R1, BUF
  mov R2, SIZE
  syscall()
  jmp :loop
end

let :exit do
  # R8 - 1
  # R0 - exit code
  mov R8, ONE
  mov R0, A
  syscall()
  ud2()
end
