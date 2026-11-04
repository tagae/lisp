# Design

A self-hosting Lisp whose compiler is written in itself and emits ARM64
executables directly.

## Goal

Two properties, deliberately both:

- **Self-hosting** — the compiler compiles its own source.
- **Toolchain-independent** — no C compiler, no assembler, no linker in the
  build: each stage compiles the next with nothing else. The only external
  dependency is a C compiler used once, to build the interpreter that starts
  the chain, plus the kernel.

Everything is as simple and dumb as possible until the fixed point is reached.
No optimization passes, no intermediate representation, no cleverness.

## Target and host

The target is Linux on a generic 64-bit ARM machine: plain ARMv8-A, ignoring
optional extensions. Output is a static ELF written directly by the compiler —
no object files, no relocations, no `PT_INTERP`, no libc. Everything the
runtime needs comes from raw syscalls. The arm64 Linux syscall ABI is the same
on every machine, so nothing in the output is specific to a board.

**The reference platform is QEMU's `virt` machine**
(`qemu-system-aarch64 -M virt`) running a stock arm64 Linux kernel. It uses a
CPU model with no extensions beyond ARMv8.0-A, such as `-cpu cortex-a57`. Not
`-cpu max`, which enables every extension QEMU knows, so code that used one
by accident would still pass. Whatever runs there should run on any arm64
Linux machine: boards, cloud instances, CI runners.

**The host can be anything.** The interpreter is portable C, so stage 1 can be
produced on any machine. Running lc1 and the later stages needs arm64 Linux,
native or emulated. `qemu-aarch64` user-mode emulation runs the static
binaries directly on a non-ARM host and is a cheap way to run the chain, but
it is not the reference: it uses the host's page size and implements the
syscalls itself.

### Page size

arm64 Linux kernels are built with 4 KB, 16 KB or 64 KB pages, and
distributions differ in which they ship. A layout that only works for 4 KB
passes on most machines and fails on the rest, so the output must work with
all three:

- The ELF has a single RWX `PT_LOAD` segment starting at file offset 0. Its
  `p_vaddr ≡ p_offset (mod page size)` holds trivially for any page size.
- If more segments are ever added, each keeps `p_vaddr ≡ p_offset (mod 64 KB)`
  and `p_align = 64 KB`, which covers 4, 16 and 64 KB kernels.
- The runtime never hardcodes 4096. Mapping sizes and addresses are rounded to
  64 KB, or to the page size read from the auxiliary vector (`AT_PAGESZ`).

## The governing principle: two independent axes

- The language the compiler is **written in** is frozen. Call it Lisp-0. It
  never grows.
- The language the compiler **accepts** grows freely.

Freezing the first axis keeps the C interpreter able to run the compiler
source indefinitely, which preserves the ability to rebuild the whole toolchain
from source forever.

**Frozen means frozen semantics, not just frozen syntax.** The compiled
compiler is compiled under the accepted language's semantics, not under
Lisp-0's. The accepted language must therefore be a strict superset of Lisp-0
that gives every Lisp-0 program exactly the same meaning. Growth may add
constructs and give meaning to programs Lisp-0 leaves undefined. It must never
change the meaning of a defined Lisp-0 program. This is why Lisp-0 leaves
fixnum overflow undefined (see below): a later switch to bignum promotion
must not change the meaning of the compiler's own code.

## Lisp-0

Deliberately minimal:

- Special forms: `quote`, `if`, `lambda`, `define`, `set!`, `begin`, plus
  `cond`, `let`, `and`, `or` and `quasiquote`, all hardcoded.
- Data: pairs, interned symbols, fixnums, strings, vectors, bytevectors, the
  empty list, and the booleans.
- Enough file I/O to read source and write a binary.

No macros in Lisp-0. This is deliberate: hardcoding roughly ten special forms
in both implementations costs less than a macro expander, and it means the
compiled compiler never has to run a transformer at compile time, so
compile-time eval drops off the critical path entirely. Macros and eval come
later, for users of the language, not for the bootstrap.

### Semantic rules

The interpreter and the compiled runtime must agree on all of these. Anything
not defined here is an error, and the compiler source never does it.

**Truthiness.** Only `#f` is false. `'()` is true and is distinct from `#f`.

**Definitions and assignment.**

- `define` appears only at top level. There is no internal `define`.
- `set!` applies only to global variables. Local variables (lambda parameters
  and `let` bindings) are immutable.

This keeps closure conversion a flat copy of captured values, with no
assignment conversion and no letrec. Local recursive helpers are written as
top-level definitions. The accepted language can lift this restriction later
by adding assignment conversion (boxing variables that are both captured and
assigned), which does not affect the bootstrap.

**Proper tail calls.** Lisp-0 has no loop construct, so every loop is
recursion, and proper tail calls are part of the language.

- Tail positions are: the body of a `lambda`; both branches of `if`; the last
  expression of `begin`, of a `cond` clause and of a `let` body; the last
  operand of `and` and `or`.
- The interpreter's `eval` loops on tail positions instead of recursing.
- The code generator tears down the frame and branches with `b`, not `bl`.

Without this, neither the interpreter nor lc1 could loop over a source file of
any size: every iteration would use a stack frame, and both run on an 8 MB
default stack.

**Identity.** `eq?` is pointer identity for heap objects and value equality for
fixnums, symbols, booleans and `'()`. The interpreter uses the runtime's tagged
words as its values, so on both sides `eq?` is a comparison of two words.

**Quoted constants are immutable.** Mutating a quoted constant is an error.
Compiled constants live in the image and interpreter constants on its heap, so
mutating one could not behave the same on both sides.

**Fixnum arithmetic.**

- Fixnums are 61-bit signed integers, stored as tagged 64-bit words with tag
  zero (the value times 8).
- Overflow is undefined, and the compiler source never relies on wraparound.
  Both sides add, subtract and multiply tagged words modulo 2⁶⁴, so an
  accidental overflow wraps at 61 bits in the same way on both sides.
- In the interpreter all of this is done on `uint64_t`. C leaves signed
  overflow, left-shifting a negative value and `INT64_MIN / -1` undefined, and
  right-shifting a negative value implementation-defined. The interpreter
  converts to `int64_t` only to compare and divide, implements arithmetic right
  shift explicitly, and never relies on compiler flags such as `-fwrapv`.
- Division and remainder truncate toward zero, which matches both ARM64 `sdiv`
  and C99 `/` and `%`. **Dividing by zero is an error, checked explicitly on both
  sides**: `sdiv` returns 0 without trapping, and in C it is undefined.
- Shift counts must be in 0–63, checked on both sides. `lslv` and `asrv` use
  the count modulo 64, and in C a shift by 64 or more is undefined. Within
  0–63, the machine and the interpreter agree.
- Both readers truncate or reject integer literals outside the 61-bit range in
  the same way.

**I/O.** The primitives cover at least:

- reading a whole file into a bytevector;
- creating a file with an explicit mode (0755 for the output binary) and
  writing a bytevector to it;
- access to the command-line arguments;
- writing to stdout and stderr;
- exiting with a status code.

## The C interpreter

Strict C99, using only the standard library (`stdio.h`, `stdlib.h`,
`string.h`, `stdint.h`, `setjmp.h`). No compiler extensions and no reliance on
compiler flags, so any C99 compiler builds it, including small ones such as
tcc. Estimated at 1,500–2,500 lines, with its own reader. It runs once to
produce stage 1 and is then kept forever, but never required.

It is a **semantic specification**, not a throwaway: any divergence from the
compiled runtime becomes a compiler that works under the interpreter and
miscompiles itself. Every rule in [Semantic rules](#semantic-rules) applies to
it.

**Same object model as the runtime.** Values are `uint64_t` tagged words with
the runtime's tags, header words and object layouts (see
[Object model](#object-model)), allocated from a bump heap. This removes whole
classes of divergence: `eq?` is a word comparison, fixnums wrap at 61 bits
because they are tagged words, and the interpreter doubles as the reference for
the heap layout that the compiled runtime can be checked against.

**Structure.**

- Global variables live in a value slot on each symbol.
- Local environments are heap frames linked to their parent.
- `eval` loops on tail positions (see
  [Proper tail calls](#semantic-rules)).
- The symbol table hashes symbol names by their contents, never by address.
- Errors print a message and `longjmp` back to the REPL, or exit with a
  non-zero status when running a file.

**Memory.** The heap is bump-allocated and nothing is freed, as in the
runtime. An interpreter allocates far more than compiled code for the same
work, though: it builds an environment frame on every call, where compiled code
mostly keeps arguments on the stack. So the self-compile may not fit. The
interpreter counts the bytes it allocates from the first day. If the
self-compile doesn't fit, the interpreter gets a mark-sweep collector over the
same heap, which doubles as a dry run of generation one.

**Stack.** The interpreter runs on the default C stack, typically 8 MB, the
same limit as compiled code. Deep non-tail recursion in the compiler source
therefore fails under the interpreter at least as early as under lc1, which is
the useful direction.

## Bootstrap chain and verification

```
interpreter + compiler.lisp  →  lc1
lc1        + compiler.lisp   →  lc2
lc2        + compiler.lisp   →  lc3
```

**Strong invariant: lc1 == lc2, byte for byte.** This holds here (unusually)
because lc1's bytes are produced by the compiler's own code generator running
under the interpreter, so the same algorithm produces them both times. It stays
available as long as the compiler's source stays in Lisp-0.

**lc2 == lc3 is a determinism check.** If lc1 == lc2, then lc3 comes from the
same binary run on the same input, so a mismatch means the compiled compiler is
nondeterministic. The usual cause is address dependence: even in a non-PIE
static binary, the stack and mmap'd regions are randomized (ASLR). Examples are
hashing or ordering by pointer, or reading an uninitialized stack slot.

**Comparing lc1, lc2 and lc3 does not show which side is at fault.** If
lc1 != lc2, the interpreter and the compiled runtime disagree on some operation
the compiler uses. lc2 == lc3 only says that the disagreement does not feed
back on itself. The fault can be on either side. For example, a miscompiled
`string<?` used to order something before emission gives a correct lc1 from the
interpreter and identical but wrong lc2 and lc3.

**Localization is by differential testing.** A corpus of small Lisp-0 programs
runs under both the interpreter and lc1 and their output is compared. The first
diverging program points at the faulty operation. Then the first differing
function in the lc1 and lc2 diff points at where it is used.

Emitted binaries must be **deterministic**: no timestamps, paths, addresses, or
iteration orders that differ between the interpreter and the compiled runtime.
The interpreter's heap addresses differ from the runtime's, so the compiler
never hashes or orders anything by address, and never iterates the symbol table
to decide emission order. Order always comes from source order or an explicit
list.

The full chain, including stage 1 and the differential corpus, runs in CI on
the reference platform, with both a 4 KB and a 64 KB page kernel.

## Object model

Committed before any code generation:

- **Tagged values**, with fixnums at tag zero, so arithmetic is a bare machine
  operation. Shared identically by the interpreter and the runtime.
- **A header word on every heap object**, carrying type and size. It is unused
  until the collector exists, which is exactly why it must be there from the
  start: a heap you can't walk is a heap no GC can work on.
- **A contiguous bump-allocated heap**: reserve a large region up front with
  lazy commit (`mmap` with `MAP_NORESERVE`, since a reservation larger than RAM
  plus swap is otherwise refused under the default overcommit mode).
  Allocating is a pointer bump and a limit check, and nothing is ever freed.

### 64-bit words are built from pieces

Fixnums hold 61 bits, but the compiler must emit 64-bit values: tagged
constants, header words and ELF fields. For example, fixnum 2⁵⁹ tags to 2⁶²,
which cannot be computed as `(* n 8)`. The assembler and ELF writer therefore
never form a full 64-bit word as a fixnum. They build words from 16-bit chunks,
which map directly onto `movz`/`movk`, or from bytes. For a tagged fixnum
`n`, chunk 0 of the tagged word is `(n << 3) & 0xFFFF` (the low bits survive
truncation), and chunk k ≥ 1 is `(n >> (16k − 3)) & 0xFFFF`, so the full
`n << 3` is never formed. For example, chunk 3 is `(n >> 45) & 0xFFFF`.

### What makes the first GC a runtime-only change

With these in place, the first GC requires zero changes to the compiler:

1. **Allocation slow path.** The emitted fast path is "bump within the current
   chunk". When the limit check fails it calls into the runtime, it does not
   exit. The limit is a variable, not an immediate. Bump allocation from free
   chunks then still works once sweeping produces them.
2. **Root region.** Global variable cells and quoted constants sit in one
   contiguous, writable region whose bounds the runtime can find from the
   image. `set!` on a global can store a heap pointer there, so this region is
   scanned as a root.
3. **Fully initialized objects.** Every object is fully initialized (fields set
   to a non-pointer such as fixnum 0, or to real values) before the next
   allocation point. The stack is scanned conservatively, but heap objects are
   marked precisely, and memory reused after a sweep holds stale pointers.

The runtime also records the stack base at `_start`, and the collector spills
callee-saved registers before scanning. Both are runtime-only.

## Code generation notes

- ARM64 instructions are uniform 4-byte bitfield encodings, so the assembler is
  an emitter over a bytevector plus a fixup list, with about 25 instruction
  encoders.
- **Branch range.** `b` and `bl` reach ±128 MB. `b.cond`, `cbz`, `cbnz` and
  `adr` reach only ±1 MB, and `tbz`/`tbnz` only ±32 KB. Conditional branches
  stay within a single function. A jump to a distant target (a shared error
  handler, the runtime) is an inverted conditional branch over an unconditional
  `b`.

## Milestones

### Generation zero — reach the fixed point

Nothing else matters until lc1 == lc2.

1. C interpreter running Lisp-0, with a REPL.
2. Assembler and ELF writer in Lisp-0. Validated by hand-emitting a trivial
   print-and-exit program, and running it under QEMU's `virt` machine on
   kernels with 4 KB and 64 KB pages.
3. Front end: reader and closure conversion. A straight recursive walk.
4. Code generator and runtime prelude. Produce lc1.
5. Grind until the invariant holds. This is expected to be most of the project.

### Generation one — memory

Mark-sweep GC with conservative scanning of the machine stack. A runtime-only
change, given the prerequisites in the
[object model](#what-makes-the-first-gc-a-runtime-only-change).

### Generation two — the language

`eval` (a small interpreter written in this Lisp and compiled in), macros for
users, a REPL over that eval, richer numerics, error handling, and a real
library. All on the accepted-language axis, and all subject to the
frozen-semantics rule.

### Generation three — reach

- A second backend (RISC-V under user-mode emulation), as the best available
  correctness audit, since it forces the front end and the machine model apart.
- Separate compilation.
- Possibly a JIT for macro expansion as an optimization, keeping the
  interpreted expander as the cross-compilation path.
- Possibly precise GC with compiler-emitted stack maps.

## FFI strategy (generation three)

Syscalls are not FFI and need no machinery. Do those first and see how far they
get.

For real C libraries:

- Write an ELF loader in Lisp rather than becoming dynamically linked.
- Generate a specialized calling stub per declared signature using the
  compiler's own code generator, rather than a libffi-style runtime marshaller.
- Restrict the supported subset to scalars and pointers: no structs by value,
  no variadics.
- Foreign memory is a distinct type from Lisp objects. Copy across the boundary
  rather than sharing.
- Callbacks need care with root scanning.

Conservative stack scanning handles roots in C frames for free, which is an
advantage. It makes a fully moving collector unavailable, which is an accepted
trade. It does not rule out a mostly-copying collector (Bartlett style), which
pins whatever the stack might reference and moves the rest.

## Designs considered and rejected

- **Emitting C** — self-hosting but not toolchain-independent, and it
  constrains tail calls.
- **A Go interpreter** — the original plan. Values as an interface with a type
  switch and environments as maps are quick to write. But they give the
  interpreter a second object model that has to be kept in step with the
  runtime: Go's `==` compares strings by contents, map iteration order is
  randomized, and integers are 64-bit. Rebuilding a modern Go from source also
  means building a long series of older Go releases, starting from Go 1.4.
- **Fexprs / quote-only, no special forms** — needs eval plus first-class
  environments, gives dynamic scoping by default, and the compiler must still
  recognize `if` syntactically to emit a branch. Wand's result makes
  optimization unsound in principle.
- **Thunk-based `if` as a plain function** — the compiler needs the
  special-form handler regardless, so it saves nothing where it matters, and it
  costs a closure per branch.
- **Lazy evaluation** — breaks the no-GC bump allocator by allocating a thunk
  per argument, and unsequenced effects are fatal for a code generator that
  appends bytes in order. Laziness belongs in the target language as
  `delay`/`force`.
