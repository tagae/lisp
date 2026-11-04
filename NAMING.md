# Naming

Choosing the language's name. This document assesses every name in the
alphabetical candidate index (about 185, counting variants), narrows them down
to a few, and records the decision.

**Status:** open.

## What needs a name

The project has more than one thing to name, and the design's two axes suggest
a scheme:

| Thing | Today | Proposed |
|---|---|---|
| The language (the accepted, growing axis) | — | **⟨Name⟩** |
| The frozen subset the compiler is written in | Lisp-0 | **⟨Name⟩-0** |
| Command | — | `⟨name⟩`, lower case |
| Source file extension | `.lisp` | `.⟨name⟩`, or a 2–3 letter abbreviation |
| Bootstrap stages | lc1, lc2, lc3 | unchanged, or ⟨n⟩c1, ⟨n⟩c2, ⟨n⟩c3 |

The suffix `-0` keeps what "Lisp-0" already says well: the frozen language is
generation zero of this language, not a separate one. A second, independent
name for the core would suggest two languages where there is one language and
its frozen subset.

## Criteria

Hard filters. A name that fails any of these is out:

1. **No live clash in the same niche.** No existing language, Lisp, compiler,
   or widely used developer tool uses the name. A clash in an unrelated field
   (music, CNC machining) is acceptable.
2. **Nothing unfortunate in English or Spanish.** Many of the candidates are
   Spanish words, so slang meanings in Spain and Latin America count.
3. **Works as a command.** Short, lower case, plain ASCII (no accents or
   umlauts), and no clash with a common Linux command.
4. **Searchable.** Not a generic English word that drowns any search for it.

Soft criteria, used to rank what survives:

5. **Fits the project.** What makes this project unusual is that the compiler
   compiles itself, that there is nothing underneath it, and that the language
   grows from a frozen core. The best names say one of these. A name that is
   merely pleasant comes second.
6. **Pleasant to type for years.** Four or five letters, ideally alternating
   hands.

Package registries such as crates.io and npm don't matter here: the toolchain
has no ecosystem dependencies, and the language will never be published
through one. Bare GitHub logins don't decide anything either, since every
plausible name is already taken as a GitHub user or organization.

## Method

All checks were made in November 2026, in three passes:

1. **Knowledge pass.** Rule out names that are generic, awkward as a command,
   or attached to a well-known language or product.
2. **GitHub pass.** Search repository names for each of about 110 remaining
   candidates combined with "language", to find language projects using the
   name.
3. **Web pass.** Search the web for the strongest survivors, and for names
   whose clashes were only remembered.

Findings marked *(from memory)* were not confirmed by a search. Confirm them
before they decide anything.

## Result

### Tier 1: strong fit, no clash found

| Name | Says | Length | Notes |
|---|---|---|---|
| **Ouro** | Self-hosting: ouroboros, the compiler eating its own tail. The fixed point `lc1 == lc2` closes the loop. | 4 | An early-stage "OuroLang" graphics language exists, with little visible activity. Portuguese and Galician for *gold*. `.ouro` has no known use. |
| **Lumbre** | The bootstrap chain: the small fire you light other fires from. The interpreter lights lc1 once and is never needed again. | 6 | Nothing found. Opaque to non-Spanish speakers. |
| **Fragua** | Toolmaking: a forge, where tools are made, which is what a self-hosting compiler is. | 6 | Nothing found. `gu` is a little awkward to type, and English speakers may not know how to say it. |

### Tier 2: good, with one specific weakness

| Name | Says | Weakness |
|---|---|---|
| Droste | Self-reference: the cocoa tin that pictures itself. As apt as Ouro. | `higherkindness/droste` is a recursion-schemes library for Scala (about 400 stars): the same functional-programming audience. A library, not a language. |
| Cauce | The governing principle: the riverbed that shapes what flows through it. | It names the frozen bed, not the river, so it fits Lisp-0 better than the growing language. The meaning is only clear in Spanish. |
| Cambium | Growth: the thin living layer where a tree grows. | Seven letters to type. A UCI research modelling language and Cambium Networks *(from memory)* share the name, but neither is in this niche. |
| Yunque | Toolmaking: an anvil. | Says less than Fragua. Six letters. Also El Yunque, the national forest in Puerto Rico. |
| Adze | Toolmaking: a shaping tool. Four letters. | Says little about this project in particular. A JavaScript logging library uses the name *(from memory)*. |
| Germ | The frozen core: the living part of the grain, where growth starts. Four letters. | Reads as *disease* first. A Rust Gemini-protocol toolkit and a Go terminal emulator use the name. |
| Chispa | The bootstrap: a spark. | Generic as a metaphor. A PySpark testing library uses the name *(from memory)*. |

### Tier 3: no clash found, but says little about the project

These survive the hard filters, but would be chosen for sound rather than
meaning:

- **Spanish:** Ancla, Bruma, Ceniza, Grano, Piedra, Semilla, Talla, Trama,
  Yunta, Zarza.
- **Craft and weaving:** Auger, Bellows, Bobbin, Rasp, Selvage, Vise.
- **Landscape and nature:** Fen, Loam, Mycel, Shale, Steppe.
- **Coinages and small words:** Baron, Carthy, Chalk, Filo, Frost, Herald,
  Kade, Kel, Nyx, Orb, Ovo, Plum, Tangle, Vell, Verba, Vole, Yolk.

Filo is in this tier rather than out: a small Go scripting language uses the
name, about the same level of clash as OuroLang, but Filo has much less to
recommend it.

### Ruled out

**Clash with an existing language, compiler or language tool.** Verified:

- Aro: a C compiler written in Zig and bundled into the Zig toolchain.
- Basalt: a small self-hosting language, plus an LLVM-based systems language.
- Corvid: a language that owns the `.cor` extension.
- Diatom: a Rust-based scripting language.
- Kip: an experimental language built on Turkish grammar, about 870 stars.
- Klein: Sun Labs' metacircular, self-hosting VM for Self. Also a
  `klein-language` GitHub organization.
- Nacre: a language for verified software, described as being in an early
  bootstrapping phase.
- Silica: a `Silica-Language-and-Compiler` GitHub project.
- Smithy: AWS's interface definition language.
- Steele: too close to Steel, an embeddable Scheme in Rust with over 2,000
  stars.
- Thorn: IBM Research and Purdue's scripting language for the JVM.
- Weft: an active language for AI systems.
- Zeta: several languages, including a self-hosting systems language with a
  Wikipedia entry, plus Zetalisp.

From memory:

- Amber (compiles to Bash), Bloom (Berkeley), Clay, Coral (CORAL 66), Crystal,
  Cypher (Neo4j), Escher, Gödel, Slate (a Smalltalk dialect), Temper, Vale,
  Vega, Vex (Houdini's shading language) and Wren are all languages.
- Kern reads as short for John Shutt's Kernel, the fexpr Lisp. It is also the
  Humdrum `**kern` music format.
- Lark is a well-known Python parser.
- Sable is SableCC and SableVM, both from the compiler world.
- Scribe was Brian Reid's markup language.
- Egg is the e-graph library used in programming-language research.
- Franz, Interlisp, Mac/Maclisp and Cadr/Cddr are existing Lisps or Lisp
  history.
- Zon is Zig's object notation format.
- Awl is a Siemens PLC language, and is one key away from `awk`, as is Auk.

**Clash with a widely used product or developer tool.**

- Verified: Graft (several developer tools), Lathe (several developer tools),
  Rime (the RIME Chinese input method), Sisu (Eclipse Sisu, used by Maven).
- From memory: Anvil, Atom, Bedrock (AWS), Boot (the Clojure build tool),
  Chisel (a hardware language), Codex, Crucible, Ember (Ember.js), Envoy,
  Flint, Forge, Granite, Halo, Kiln, Kindle, Loom, Mise (the `mise` tool
  manager, a direct command clash), Mono, Orin, Quartz, Quill, Rook, Rosetta,
  Root, Sapling (Meta's version control system), Skein, Spark, Substrate,
  Tinder, Turtle (the RDF syntax), Warp, Wheel.

**Generic or unsearchable.**

- Alpha, Bare, Bud, Coin, Cons, Core, Dawn, Eval, First, Fix/Fixpoint, Gen/Gensym,
  Genesis, Ground/Groundwork, Grow, Lexis, Loop, Meta, Naked, Nihil/Nil, Nub,
  Null, Origin, Paren, Pilot, Prime, Quasi, Quine, Raw, Seed, Sexp, Shell,
  Shoot, Small, Sprout, Stage, Stem, Strange, Strap, Strata, Stub, Sym, Trad,
  Vector, Vocab, Word, Y, Zero.
- Also generic: Crystal, Fractal and Dragon, besides their other clashes.
- Lisp-0 stays as the frozen subset's name, or becomes ⟨Name⟩-0. It is not the
  language's name.

**Awkward as a command.**

- Accents or umlauts: Ámbar, Cuña, Gödel, Münch, Raíz, Vórtice. Dropping the
  accent changes the word.
- Too long: Hofstadter, Meristem, Monosyl, Mycelium, Rootstock.

**Unfortunate meaning.**

- Yesca: slang for marijuana in Central America and parts of Mexico and
  Colombia.
- Brote: also means *outbreak*, as in *brote de gripe*.
- Vaina: colloquial for *thing* or *stuff* in Colombia and Venezuela, often
  as a mild curse.
- Slag and Guy: both already flagged in the index as jokes.
- Ptarmigan: withdrawn in the index.

## Recommendation

**Ouro**, with **Lumbre** and **Fragua** as the alternatives.

- **Ouro** is the only clean name that describes the project's defining
  property, a compiler that compiles itself, rather than a metaphor around it.
  It is also the shortest. Ouro-0 for the frozen core reads naturally as "the
  loop at generation zero". The thing to watch is the small OuroLang project.
- **Lumbre** is the choice if the name should describe the bootstrap chain:
  one fire lit once, from which everything else is lit.
- **Fragua** is the choice if the name should describe what the compiler is:
  the place where the tools are made.

Droste is as apt as Ouro. It drops to tier 2 only because a well-known Scala
library has the name, which matters more than the small OuroLang project
because both reach the same audience. If you accept that clash, it is a
fourth finalist.

## How to decide

1. Confirm the *(from memory)* findings for any name you still want to keep.
2. Search for "⟨name⟩ language" and "⟨name⟩ lisp" one more time, in case
   something new has appeared.
3. On a typical Linux system, check that nothing else is installed under the
   command name:
   `command -v ⟨name⟩` and `apt-cache policy ⟨name⟩`.
4. Type `⟨name⟩ build compiler.⟨ext⟩` a few times, and keep whichever you
   don't mind typing on a bad day.
5. Record the decision below. Then rename Lisp-0 to ⟨Name⟩-0 in
   [DESIGN.md](DESIGN.md), and decide whether the stage binaries keep the
   `lc` prefix.

## Decision

- **Name:** —
- **Frozen subset:** —
- **Command:** —
- **File extension:** —
- **Decided on:** —
- **Why:** —

## Sources

- Ouro: [OuroLang syntax documentation](https://donalddarkness.github.io/lava)
- Droste: [higherkindness/droste](https://vic.github.io/awesome-gh-repos/stars/droste), [quasarbright/droste](https://github.com/quasarbright/droste/releases)
- Germ: [gemrest/germ](https://awesome.ecosyste.ms/projects/github.com%2Fgemrest%2Fgerm), [ishuah/germ](https://github.com/ishuah/germ)
- Cambium: [Cambium-Plenum Translator (UCI)](https://computableplant.ics.uci.edu/sw/CambiumPlenum/)
- Filo: [crgimenes/filo on pkg.go.dev](https://pkg.go.dev/github.com/crgimenes/filo)
- Aro: [Vexu/arocc](https://github.com/Vexu/arocc)
- Basalt: [memeviber/Basalt](https://github.com/memeviber/Basalt), [fDero/Basalt](https://github.com/fDero/Basalt)
- Corvid: [Corros README, on retiring `.cor`](https://github.com/CocoCopi/corros)
- Diatom: [The Diatom Programming Language](https://docs.rs/crate/diatom/latest)
- Kip: [kip-dili/kip](https://github.com/kip-dili/kip)
- Klein: [OOPSLA 2005, "Constructing a Metacircular Virtual Machine in an Exploratory Programming Environment"](https://www.doi.org/10.1145/1094855.1094865), [Live Objects All The Way Down (2023)](https://arxiv.org/pdf/2312.16973)
- Kern: [Humdrum \*\*kern](https://kernpy.pages.dev/concepts/humdrum-kern)
- Nacre: [aerkiaga/nacre](https://awesome.ecosyste.ms/projects/github.com%2Faerkiaga%2Fnacre)
- Steel: [mattwparas/steel](https://github.com/mattwparas/steel)
- Thorn: [Thorn: robust, concurrent, extensible scripting on the JVM](https://research.ibm.com/publications/thorn-robust-concurrent-extensible-scripting-on-the-jvm)
- Weft: [WeaveMindAI/weft](https://github.com/WeaveMindAI/weft)
- Zeta: [Zeta (programming language)](https://en.wikipedia.org/wiki/Zeta_(programming_language)), [frogfishio/zeta](https://github.com/frogfishio/zeta)
- Graft: [NanoNets/Graft](https://github.com/NanoNets/Graft), [graft on opam](https://opam-4.ocaml.org/packages/graft)
- Lathe: [lathe-cli/lathe](https://github.com/lathe-cli/lathe), [devenjarvis/lathe](https://github.com/devenjarvis/lathe)
- Rime: [librime](https://github.com/biaoyinzi/librime)
- Sisu: [Eclipse Sisu proposal](https://eclipse.org/proposals/technology.sisu)
- Yesca: [SpanishDict, "yesca"](https://www.spanishdict.com/translate/yesca)
