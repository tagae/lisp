# Design

A self-hosting functional Lisp whose compiler is written in itself and emits
ARM64 executables directly.

## Objectives

**What it is.** A functional language of the Lisp family: immutable data by
default, persistent data structures, composition over side effects. It draws
on Clojure, Common Lisp and Go, uses its own names, and is source-compatible
with none of them.

**Why self-hosting.**

- A compiler that compiles itself is the proof that the language is not a toy.
- Bootstrapping stays simple: a small C compiler such as tcc is enough to
  start the chain.
- Every byte between source and executable is under the project's control and
  can be understood.

**What comes with it.** A language reference, readable error messages and a
one-command build from source.

**Bootstrap goals.**

- **Self-hosting** — the compiler compiles its own source.
- **Toolchain-independent** — no C compiler, no assembler, no linker in the
  build: each stage compiles the next with nothing else. The only external
  dependency is a C compiler, to build the interpreter that starts the chain,
  plus the kernel.
- **A minimal C interpreter** — the interpreter is the only part of the
  project that is not written in the language itself, so it is kept as small
  as possible. Whatever can be written in Lisp-0 is, and Lisp-0 has nothing
  the compiler can do without (see [The C interpreter](#the-c-interpreter)).

**The bootstrap comes first.** When a language feature conflicts with the
bootstrap, the feature gives way: it is simplified, moved to the second
compiler, or dropped.

Everything is as simple and dumb as possible until the fixed point is reached.
No optimization passes, no intermediate representation, no cleverness.

**Language goals**, in the order they are built after the fixed point (see
[Milestones](#milestones)):

- garbage collection, macros and `eval`;
- a standard library with persistent collections, written in the full
  language. Since the compiler itself stays in Lisp-0, this library is the
  proof that the full language is not a toy;
- conditions and restarts, as in Common Lisp: handlers run at the point of the
  error, without unwinding the stack, and can pick a restart;
- gradual typing: annotations are optional, checked at compile time where
  present, and untyped code stays valid;
- an optimizing compiler, at which point the performance of generated code
  becomes an objective;
- true parallelism: lightweight threads and channels running on several cores;
- simplicity and tooling in the spirit of Go: a small language, fast compiles,
  a formatter and a test runner, single static binaries;
- datoms and a Datomic-style database, as a library.

**Non-goals.**

- Source compatibility with Clojure, Scheme or Common Lisp.
- macOS and Windows, which cannot be targeted without the vendor's toolchain.
- Any target other than ARM64 Linux before the standard library exists.
- Auditing the bootstrap below the C compiler. tcc is small enough.

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
- The language the compiler **accepts** grows, up to a fixed limit (see
  [Two compilers](#two-compilers)).

Freezing the first axis keeps the C interpreter able to run the compiler
source indefinitely, which preserves the ability to rebuild the whole toolchain
from source forever. **Every build goes through the interpreter**: no compiled
compiler is ever committed or downloaded.

**Frozen means frozen semantics, not just frozen syntax.** The compiled
compiler is compiled under the accepted language's semantics, not under
Lisp-0's. The accepted language must therefore be a strict superset of Lisp-0
that gives every Lisp-0 program exactly the same meaning. Growth may add
constructs and give meaning to programs Lisp-0 leaves undefined. It must never
change the meaning of a defined Lisp-0 program. This is why Lisp-0 leaves
fixnum overflow undefined and rejects integer literals that do not fit a
fixnum (see below): a later switch to bignum promotion must not change the
meaning of the compiler's own code.

**Lisp-0 is defined in full before any code is written, and frozen at the
fixed point.** Between the two it can be amended, each change with its reason
recorded here.

### Two compilers

The compiler written in Lisp-0 is the **bootstrap compiler**. The
byte-for-byte invariant applies to it alone.

It accepts a **middle language**, defined once: Lisp-0 plus macros, local
mutation, rest parameters and whatever data types the second compiler needs.
Once it accepts the middle language, the bootstrap compiler stops growing.
This bounds the time and memory of stage 1 by design: stage 1 runs the whole
bootstrap compiler under a plain tree-walking interpreter, and the interpreter
is not made faster to keep up.

The **second compiler** is written in the middle language and compiled by the
bootstrap compiler. It implements the full language: the richer syntax,
conditions, typing, optimization and parallelism. It reaches its own fixed
point in the ordinary way (it compiles itself twice and the two outputs are
identical).

## Lisp-0

Lisp-0 is a subset of the functional language, not a Scheme that the language
later moves away from. Its rules on data, truthiness and syntax are therefore
those of the full language.

It contains only what the compiler cannot reasonably do without, because
every form and primitive is implemented twice: in C in the interpreter, and in
the code generator.

**Special forms**, all hardcoded: `quote`, `if`, `fn`, `def`, `set!`, `do`,
`let`, `cond`, `and`, `or`. The bodies of `fn` and `let` and the clauses of
`cond` are implicit `do`s.

**Data**: pairs, interned symbols, fixnums, strings, bytevectors, functions
and `nil`.

**Left out**, and why:

- *Macros.* Hardcoding ten special forms in both implementations costs less
  than a macro expander, and the compiled compiler never has to run a
  transformer at compile time, so compile-time eval drops off the critical
  path.
- *`quasiquote`.* It needs three more reader characters and a recursive
  special form on both sides. The compiler builds lists with `pair`.
- *Vectors.* Pairs and bytevectors are enough for the compiler. Vectors come
  with the middle language.
- *Booleans and a separate empty list.* See [Truthiness](#semantic-rules).
- *A character type, and anything outside ASCII.*
- *Rest parameters and `apply`.*
- *Structural equality.*
- *A loop construct.*

**Names** follow one rule: Clojure's short form where it has a good one (`fn`,
`def`, `do`, `let`), whole English words otherwise (`first` and `rest`, not
`car` and `cdr`), `?` at the end of a predicate and `!` at the end of a
mutator. The names in this document are those the rule gives. They are
confirmed one by one in step 0 of [Milestones](#milestones).

### Semantic rules

The interpreter and the compiled runtime must agree on all of these. Anything
not defined here is an error, and the compiler source never does it.

**Truthiness.** `nil` is the only falsy value, and it is also the empty list:
a list is `nil` or a pair. There are no booleans. Predicates return the symbol
`t`, whose value is itself and cannot be changed. Every value other than `nil`
is true, including the empty string, the empty bytevector and zero. `if` is
therefore a comparison with one word.

`first` and `rest` of `nil` are `nil`. Of anything else that is not a pair
they are an error.

**Immutable data.** Pairs, strings and symbols are immutable: there is no
`set-first!` and no `string-set!`. A string of a length not known in advance
is accumulated in a bytevector and converted once, by `bytes->string`.
Mutation exists only in two explicit places:

- bytevectors, which are mutable buffers (the assembler writes into one and
  patches fixups in it, and the compiler's tables are built on them);
- global variables, through `set!`.

**Strings.** A string is an immutable sequence of bytes, each in 0–127.
Indexing gives a fixnum. Lisp-0 source is ASCII: both readers reject any byte
above 127, so the full language is free to decide what a character is.

**Symbol alphabet and reserved syntax.** A Lisp-0 symbol contains only
letters, digits and `- ? ! * + / < > = .`, and does not look like an integer.
Besides these, the reader accepts `(`, `)`, `'`, `"` and `;`. Every other
ASCII punctuation character is rejected outside strings and comments. That
reserves `[ ] { } : # @ ^ ~ & % $ | \ _`, the backquote and the comma, so the
later languages can give them a meaning without changing any Lisp-0 program.

**Literals.** Integers are written in decimal or, with `0x`, in hexadecimal.
A literal that does not fit a fixnum is an error. In a string, `\n`, `\\` and
`\"` are the only escapes.

**Definitions and assignment.**

- `def` appears only at top level. There is no internal `def`.
- `set!` applies only to global variables. Local variables (`fn` parameters
  and `let` bindings) are immutable.
- The bindings of a `let` do not see each other.

This keeps closure conversion a flat copy of captured values, with no
assignment conversion and no letrec. Local recursive helpers are written as
top-level definitions. The middle language lifts this restriction by adding
assignment conversion (boxing variables that are both captured and assigned),
which does not affect the bootstrap.

**Fixed arity.** A function has a fixed number of parameters, and calling it
with another number is an error. This also holds for primitives: `+` takes
two operands.

**Proper tail calls.** Lisp-0 has no loop construct, so every loop is
recursion, and proper tail calls are part of the language.

- Tail positions are: the last expression of a `fn` body, of `do`, of a
  `cond` clause and of a `let` body; both branches of `if`; the last operand
  of `and` and `or`.
- The interpreter's `eval` loops on tail positions instead of recursing.
- The code generator tears down the frame and branches with `b`, not `bl`.

Without this, neither the interpreter nor lc1 could loop over a source file of
any size: every iteration would use a stack frame, and both run on an 8 MB
default stack.

**Identity is the only equality.** `identical?` compares two words: pointer
identity for heap objects, value equality for fixnums, symbols and `nil`. The
interpreter uses the runtime's tagged words as its values, so both sides do
the same comparison. Comparing strings by content is a Lisp-0 function in the
prelude.

**Quoted constants are immutable.** A literal can only be a pair, a symbol, a
fixnum, a string or `nil`, all of which are immutable. There is no bytevector
literal.

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
  and C99 `/` and `%`. Dividing by zero is an error: `sdiv` returns 0 without
  trapping, and in C it is undefined.
- Shift counts must be in 0–63. `lslv` and `asrv` use the count modulo 64, and
  in C a shift by 64 or more is undefined.

**Errors.** Both sides check the type of every operand, every index, every
argument count, division by zero and shift counts. Lisp-0 has a fixed table of
errors, each with one message and one exit status. On an error, both sides
write exactly that message to stderr and exit with that status: no values, no
locations. A failing program therefore behaves identically on both sides, and
the differential corpus compares failures as well as successes. Errors with
more detail belong to the later languages.

### Primitives

About thirty, and each exists because it cannot be written in Lisp-0:

| Group | Primitives |
|---|---|
| Pairs | `pair`, `first`, `rest`, `pair?` |
| Types and identity | `identical?`, `symbol?`, `string?`, `bytes?`, `integer?` |
| Arithmetic | `+`, `-`, `*`, `quotient`, `remainder`, `<` |
| Bits | `bit-and`, `bit-or`, `shift-left`, `shift-right` |
| Strings | `string-length`, `string-byte`, `bytes->string` |
| Bytevectors | `bytes`, `bytes-length`, `bytes-get`, `bytes-set!` |
| Symbols | `symbol`, `symbol-name` |
| System | `read-file`, `write-file`, `write`, `arguments`, `exit` |

The last two groups are the **duals** (see below). Everything else that the
compiler uses (list functions, string comparison, number parsing and printing,
hash tables, formatted output) is Lisp-0 code in the prelude.

**Compile-only primitives.** Lisp-0 also defines `syscall` and raw memory
access (read and write a word or a byte at an address, the address of an
object, the stack pointer). The compiler emits them inline. They have no
meaning under the interpreter, which does not know them: only the runtime uses
them, and the interpreter never loads the runtime. Their set is frozen with
the rest of Lisp-0, so it must be shown to be enough for a collector before
the freeze (see [Milestones](#milestones)).

## Source files and who runs them

The compiler's source is in three parts.

- **The prelude**: Lisp-0 library code. The interpreter loads it, and the
  compiler compiles it into every image. One source for both sides, so it
  cannot diverge.
- **The compiler**: reader, closure conversion, code generator, assembler and
  ELF writer. Loaded by the interpreter, compiled into lc1.
- **The runtime**: Lisp-0 code that uses the compile-only primitives. The
  interpreter never loads it. The compiler reads it as data and compiles it
  into every image. It has two kinds of functions:
  - *the duals' bodies*: the Lisp-0 implementation, over syscalls, of each
    system and symbol primitive of the table above. The interpreter implements
    the same primitives in C, with the standard library;
  - *runtime-internal functions*, which no Lisp-0 program calls by name and
    which have no counterpart in C: start-up, heap setup, the allocation slow
    path, the error exits, and later the collector.

**The duals are the one place where two implementations of the same function
are written by hand**, so there are as few as possible and their contracts are
as narrow as possible:

- `read-file` returns a whole file as a bytevector. `write-file` creates a
  file from a whole bytevector. `write` sends a bytevector to stdout or
  stderr. `arguments` returns the command-line arguments as a list of strings.
  `exit` ends the program with a status.
- `symbol` returns the unique symbol with a given name, and `symbol-name` its
  name. In compiled code the symbol table is a hash table over bytevectors,
  hashed by the contents of the name and seeded at start-up from the symbols
  in the image.
- A dual succeeds completely or fails with an error from the table. It never
  returns an error value, so a Lisp-0 program cannot observe a failure.
- Their test is the self-compile: if the two sides differ, lc1 and lc2 differ.

**One known difference.** In compiled code `write-file` creates the file with
mode 0755. C99 has no way to set a mode, so the file the interpreter creates
is not executable, and the build marks lc1 executable itself. No Lisp-0
program can observe the difference.

## The C interpreter

Strict C99, using only `stdio.h`, `stdlib.h`, `string.h` and `stdint.h`. No
compiler extensions, no reliance on compiler flags, no POSIX, so any C99
compiler builds it on any host. It is needed for every build, forever.

**It is as small as it can be**: about 1,000–1,500 lines. What keeps it small:

- **It only runs files.** `interpreter file... -- argument...` evaluates the
  top-level forms of each file in order and exits. There is no REPL.
- **Errors exit.** An error writes its message from the table and exits with
  its status. There is no recovery, so no `setjmp`.
- **No collector.** The heap is one block obtained from `malloc` and
  bump-allocated, and nothing is freed. Running out is an error from the
  table. If the self-compile does not fit, the compiler's source changes, not
  the interpreter.
- **No optimization.** A variable is looked up by walking the environment
  frames each time. If stage 1 is too slow, the compiler's source changes.
- **A reader for Lisp-0 only**: parentheses, `'`, symbols, integers, strings
  and comments.
- **Ten special forms and about thirty primitives**, of fixed arity.
- **Nothing that Lisp-0 code can do.** No list library, no string comparison,
  no number formatting: these are in the prelude.

**Building with tcc is a requirement, checked in CI.** The interpreter is
built with tcc and with gcc or clang, and the lc1 each build produces must be
byte-identical. This turns "any C99 compiler" into a tested claim, and keeps
the bootstrap within reach of a compiler far smaller than gcc.

It is a **semantic specification**, not a throwaway: any divergence from the
compiled runtime becomes a compiler that works under the interpreter and
miscompiles itself. Every rule in [Semantic rules](#semantic-rules) applies to
it.

**Same object model as the runtime.** Values are `uint64_t` tagged words with
the runtime's tags, header words and object layouts (see
[Object model](#object-model)), allocated from the bump heap. This removes
whole classes of divergence: `identical?` is a word comparison, fixnums wrap
at 61 bits because they are tagged words, and the interpreter doubles as the
reference for the heap layout that the compiled runtime can be checked
against.

**Structure.**

- Global variables live in a value slot on each symbol.
- Local environments are heap frames linked to their parent.
- `eval` loops on tail positions (see
  [Proper tail calls](#semantic-rules)).
- The symbol table hashes symbol names by their contents, never by address.

**Stack.** The interpreter runs on the default C stack, typically 8 MB, the
same limit as compiled code. Deep non-tail recursion in the compiler source
therefore fails under the interpreter at least as early as under lc1, which is
the useful direction.

## Bootstrap chain and verification

```
interpreter + prelude, compiler  →  lc1
lc1         + compiler source    →  lc2
lc2         + compiler source    →  lc3
```

The interpreter loads the prelude and the compiler, and the compiler then
compiles the prelude, the runtime and itself.

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
the compiler uses: a primitive, a special form or a dual. lc2 == lc3 only says
that the disagreement does not feed back on itself. The fault can be on either
side. For example, a miscompiled `<` used to order something before emission
gives a correct lc1 from the interpreter and identical but wrong lc2 and lc3.

**Localization is by differential testing.** A corpus of small Lisp-0 programs
runs under both the interpreter and lc1, and their output, their stderr and
their exit status are compared. The first diverging program points at the
faulty operation. Then the first differing function in the lc1 and lc2 diff
points at where it is used.

Emitted binaries must be **deterministic**: no timestamps, paths, addresses, or
iteration orders that differ between the interpreter and the compiled runtime.
The interpreter's heap addresses differ from the runtime's, so the compiler
never hashes or orders anything by address, and never iterates the symbol table
to decide emission order. Its own tables hash names by their contents. Order
always comes from source order or an explicit list.

The full chain, including stage 1 and the differential corpus, runs in CI on
the reference platform, with both a 4 KB and a 64 KB page kernel, and with the
interpreter built by tcc as well as by gcc or clang.

## Object model

Committed before any code generation:

- **Tagged values**, with fixnums at tag zero, so arithmetic is a bare machine
  operation. `nil` is an immediate word. Shared identically by the interpreter
  and the runtime.
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

The collector is a runtime-internal function, written in Lisp-0 with the
compile-only primitives. It must not allocate while it runs, so the definition
of Lisp-0 states which constructs are guaranteed not to allocate.

### Room reserved for later generations

Parallelism, conditions and typing are objectives that generation zero does
not implement. It reserves only what is nearly free now and expensive to
change later:

- **The allocation pointer and limit live in registers reserved for the
  current thread**, not in global variables. Emitted code never assumes that
  there is one allocator state for the whole process.
- **Nothing assumes a single stack.** The stack base is a field of per-thread
  runtime state reached through a reserved register, not a global.
- **Syntax is reserved in the reader** (see
  [Symbol alphabet](#semantic-rules)).

Nothing else about those features is designed until their generation.

## Code generation notes

- ARM64 instructions are uniform 4-byte bitfield encodings, so the assembler is
  an emitter over a bytevector plus a fixup list, with about 25 instruction
  encoders.
- **Branch range.** `b` and `bl` reach ±128 MB. `b.cond`, `cbz`, `cbnz` and
  `adr` reach only ±1 MB, and `tbz`/`tbnz` only ±32 KB. Conditional branches
  stay within a single function. A jump to a distant target (a shared error
  handler, the runtime) is an inverted conditional branch over an unconditional
  `b`.
- **Lookup tables** (global slots, labels, constants) are open-addressing hash
  tables built in Lisp-0 over bytevectors, keyed by the contents of names.
- **The shape of generated code and the calling convention are decided at
  step 4**, once the assembler exists.

## Milestones

### Generation zero — reach the fixed point

Nothing else matters until lc1 == lc2.

0. Finish the definition of Lisp-0: confirm each name, write the error table,
   and state which constructs do not allocate. The interpreter cannot start
   before this.
1. C interpreter running Lisp-0 files. Builds with tcc.
2. Prelude, assembler and ELF writer in Lisp-0. Validated by hand-emitting a
   trivial print-and-exit program, and running it under QEMU's `virt` machine
   on kernels with 4 KB and 64 KB pages.
3. Front end: reader and closure conversion. A straight recursive walk.
4. Code generator and runtime. Produce lc1.
5. Grind until the invariant holds.
6. Write a throwaway mark-sweep collector in the runtime, to show that the
   compile-only primitives are enough for one. Then freeze Lisp-0.

### Generation one — memory

Mark-sweep GC with conservative scanning of the machine stack. A runtime-only
change, given the prerequisites in the
[object model](#what-makes-the-first-gc-a-runtime-only-change).

### Generation two — the two languages

1. **The middle language.** The bootstrap compiler learns macros (with a small
   evaluator written in Lisp-0 and compiled in), local mutation, rest
   parameters and vectors. It then stops growing.
2. **The second compiler**, written in the middle language. At first it
   accepts the middle language and optimizes nothing.
3. **The full language**, in the second compiler: `eval`, a REPL over it,
   richer numerics, and the richer syntax: brackets, braces and other sugar,
   in the manner of Clojure.
4. **The standard library** with persistent collections, written in the full
   language, and the language reference.

All of it is subject to the frozen-semantics rule.

### Generation three — the larger features

All in the second compiler, in this order:

1. **Conditions and restarts.** First, because the library's error behaviour
   depends on them. In Lisp-0 an error still just exits, so this is additive.
2. **Gradual typing.** Before the optimizer, which uses the types.
3. **Optimization.** Performance becomes an objective here.
4. **True parallelism.** Last, because it forces the largest rework of the
   runtime and the collector. Conservative scanning of many stacks with a
   stop-the-world collector is the expected starting point, and the riskiest
   part of the plan.

### Generation four — reach

- A second backend (RISC-V under user-mode emulation), as the best available
  correctness audit, since it forces the front end and the machine model apart.
- Separate compilation.
- Tooling: a formatter and a test runner.
- Datoms and a Datomic-style database, as a library.
- Possibly a JIT for macro expansion as an optimization, keeping the
  interpreted expander as the cross-compilation path.
- Possibly precise GC with compiler-emitted stack maps.

## FFI strategy (generation four)

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
- **Syscalls in the interpreter** — it would let the interpreter run the
  runtime's Lisp-0 code, with no duals. But a raw syscall cannot be written in
  portable C99: it needs libc's `syscall` or inline assembly, and syscall
  numbers differ between architectures, so the interpreter would stop running
  on any host.
- **A REPL and error recovery in the interpreter** — convenient, but the
  bootstrap only ever runs files.
- **A collector or pre-resolved variables in the interpreter** — they would
  let the bootstrap compiler grow further. Instead the bootstrap compiler
  stops at the middle language.
- **Marking the duals in the source**, with a special form or a rule that a
  builtin wins over a definition of the same name — both need code in the
  interpreter. Keeping the duals' bodies in the runtime, which the interpreter
  never loads, needs none.
- **Fexprs / quote-only, no special forms** — needs eval plus first-class
  environments, gives dynamic scoping by default, and the compiler must still
  recognize `if` syntactically to emit a branch. Wand's result makes
  optimization unsound in principle.
- **Thunk-based `if` as a plain function** — the compiler needs the
  special-form handler regardless, so it saves nothing where it matters, and it
  costs a closure per branch.
- **Three distinct values `nil`, `false` and the empty list**, as in Clojure —
  it keeps "absent" apart from "empty" and from "no". A single `nil` is one
  kind of value fewer on both sides, and makes every test one comparison.
- **Falsy empty strings and bytevectors** — it would turn every `if` from a
  word comparison into a type and count check.
- **Moving the compiler to the full language** after the fixed point — it would
  let the compiler use the language's own features, but rebuilding from source
  would then need a chain of older compilers, and lc1 == lc2 would be lost.
- **A bootstrap compiler that accepts the full language** — it would grow with
  every feature, and stage 1 with it.
- **Fully static typing** — Lisp-0 would have to be typeable from the first
  day, so the type system would have to be designed before any code.
- **Lazy evaluation** — breaks the no-GC bump allocator by allocating a thunk
  per argument, and unsequenced effects are fatal for a code generator that
  appends bytes in order. Laziness belongs in the target language as
  `delay`/`force`.
