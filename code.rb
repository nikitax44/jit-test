require_relative "macros.rb"

A = 13
B = 14
BUF = 16
SIZE = 17
CHAR_0 = 18
TEN = 19


let :main do
  movc BUF, 128
  movc TEN, 10
  movc CHAR_0, '0'.ord

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
end
