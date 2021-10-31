
import sys

#a = [0 for a in range(30000)]


ptr = 0
#  84 LOAD_CONST               0 (0)
#  86 STORE_NAME               7 (ptr)

ptr += 1
#  88 LOAD_NAME                7 (ptr)  - Pushes the value associated with co_names[namei] onto the stack.
#  90 LOAD_CONST               3 (1)    - Pushes co_consts[consti] onto the stack.
#  92 INPLACE_ADD
#  94 STORE_NAME               7 (ptr)  - Implements name = TOS (and pops the TOS)

ptr -= 1
#  96 LOAD_NAME                7 (ptr)
#  98 LOAD_CONST               3 (1)
# 100 INPLACE_SUBTRACT
# 102 STORE_NAME               7 (ptr)

a[ptr] += 1
# 104 LOAD_NAME                1 (a)
# 106 LOAD_NAME                7 (ptr)
# 108 DUP_TOP_TWO                       - Duplicates the two references on top of the stack, leaving them in the same order.
# 110 BINARY_SUBSCR                     - Implements TOS = TOS1[TOS].
# 112 LOAD_CONST               3 (1)
# 114 INPLACE_ADD                       - Implements in-place TOS = TOS1 + TOS.
# 116 ROT_THREE                         - Lifts second and third stack item one position up, moves top down to position three.
# 118 STORE_SUBSCR                      - Implements TOS1[TOS] = TOS2.

a[ptr] -= 1
# same just with INPLACE_SUBTRACT

sys.stdout.write(a[ptr])
# 136 LOAD_NAME                0 (sys)
# 138 LOAD_ATTR                8 (stdout)  - Replaces TOS with getattr(TOS, co_names[namei]).
# 140 LOAD_METHOD              9 (write)   - Loads a method named co_names[namei] from the TOS object. TOS is popped.
# 142 LOAD_NAME                1 (a)
# 144 LOAD_NAME                7 (ptr)
# 146 BINARY_SUBSCR
# 148 CALL_METHOD              1      - Calls a method. argc is the number of positional arguments
# 150 POP_TOP                         - we don't care about the return value

sys.stdout.write(chr(a[ptr]))
# 60 LOAD_NAME                0 (sys)
# 62 LOAD_ATTR                3 (stdout)
# 64 LOAD_METHOD              4 (write)
# 66 LOAD_NAME                5 (chr)
# 68 LOAD_NAME                2 (a)
# 70 LOAD_NAME                1 (ptr)
# 72 BINARY_SUBSCR
# 74 CALL_FUNCTION            1
# 76 CALL_METHOD              1
# 78 POP_TOP

a[ptr] = ord(sys.stdin.read(1))
#  96 LOAD_NAME                6 (ord)
#  98 LOAD_NAME                0 (sys)
# 100 LOAD_ATTR                7 (stdin)
# 102 LOAD_METHOD              8 (read)
# 104 LOAD_CONST               2 (1)
# 106 CALL_METHOD              1
# 108 CALL_FUNCTION            1
# 110 LOAD_NAME                2 (a)
# 112 LOAD_NAME                1 (ptr)
# 114 STORE_SUBSCR


if a[b]:
	pass

# 116 LOAD_NAME                2 (a)
# 118 LOAD_NAME                9 (b)
# 120 BINARY_SUBSCR
# 122 POP_JUMP_IF_FALSE      130      - If TOS is false, sets the bytecode counter to target. TOS is popped.

#  ... }
# 116 LOAD_NAME                2 (a)
# 118 LOAD_NAME                9 (b)
# 120 BINARY_SUBSCR
# 122 POP_JUMP_IF_TRUE      100      - If TOS is true, sets the bytecode counter to target. TOS is popped.

asa = 0
basdf = [asa, 0, 0, 0]

# 148 LOAD_CONST               0 (0)
# 150 LOAD_CONST               0 (0)
# 152 LOAD_CONST               0 (0)
# 154 BUILD_LIST               3
# 156 STORE_NAME              12 (basdf)
