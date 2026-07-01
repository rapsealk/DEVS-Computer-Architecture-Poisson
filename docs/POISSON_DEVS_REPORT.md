# A Poisson Arrival Process in DEVS — Model Report

This report explains the example added in this repository: a single-server
queueing model built on the **DEVS** formalism, whose job arrivals are driven by
a **Poisson distribution**. It covers the formalism, the model structure, the
role of the Poisson generator, a walkthrough of an actual simulation trace, and
what the results demonstrate.

---

## 1. Background: what DEVS is and why it matters here

**DEVS (Discrete Event System Specification)** is a formalism, introduced by
Bernard Zeigler, for modelling systems whose state changes only at discrete
instants in time. Its defining characteristic is that simulation is
**event-driven, not clock-driven**: rather than advancing time in fixed steps and
asking "did anything change?", a DEVS simulator computes *when the next event
will occur* and jumps the global clock straight to that instant.

This matters for the present example because job arrivals happen at irregular,
random moments. A fixed-step simulator would waste effort ticking through empty
intervals; DEVS advances directly from one arrival (or service completion) to the
next.

A DEVS model is built from two kinds of components:

- **Atomic models** — indivisible behavioural units. Each is defined by exactly
  four functions and one state variable, `Sigma` (the time remaining until its
  next internal event).
- **Coupled models** — structures that compose atomic (and other coupled) models
  by connecting their input and output **ports**.

### The atomic-model contract

| Function | Trigger | Responsibility |
|----------|---------|----------------|
| `InitializeFN` | simulation start | set initial phase/state and first `Sigma` |
| `OutputFN` | just **before** an internal transition | emit output messages |
| `IntTransitionFN` | `Sigma` elapses (internal event) | update state, schedule next `Sigma` |
| `ExtTransitionFN` | an input message arrives (external event) | react to the event |

The key mechanism is **`HoldIn(phase, duration)`**, which sets the phase and
schedules the next internal event `duration` time units into the future
(`Sigma = duration`). `Passivate()` means "wait indefinitely" (`Sigma = ∞`),
and `Continue()` keeps the current schedule.

---

## 2. The model: the `ef-p` single-server queue

The example builds the classic **experimental-frame / processor** (`ef-p`) model.
It is the "hello world" of queueing simulation.

```
                 ef  (experimental frame)
        ┌────────────────────────────────────────┐
        │   ┌───────────┐  arriv   ┌───────────┐  │
  OUT ◀─┼───│ Generator │─────────▶│Transducer │  │
        │   │  (genr)   │          │ (transd)  │  │
        │   └─────┬─────┘          └─────▲─────┘  │
        │         │ out                  │ solved │
        └─────────┼──────────────────────┼────────┘
             IN   ▼                       │ OUT
        ┌────────────────────────────────────────┐
        │                 Process                 │
        │              (single server)            │
        └────────────────────────────────────────┘
```

| Component | Source | Role |
|-----------|--------|------|
| **Generator** (`genr`) | [`src/Generator.cpp`](../DEVS/src/Generator.cpp) | Emits jobs `Job-0`, `Job-1`, …. The gap between successive jobs is a Poisson draw. |
| **Process** | [`src/Process.cpp`](../DEVS/src/Process.cpp) | A single-server queue. Buffers jobs and serves them one at a time, each taking `PTime = 7.0` time units. |
| **Transducer** (`transd`) | [`src/Transducer.cpp`](../DEVS/src/Transducer.cpp) | Observes arrivals and completions for a window of `100.0` units, then prints a summary. |

The wiring lives in [`src/main.cpp`](../DEVS/src/main.cpp) and is expressed purely
as port couplings — the components have no direct knowledge of one another:

```cpp
efp->AddCouple("ef", "Process", "OUT", "in");   // frame → processor input
efp->AddCouple("Process", "ef", "out", "IN");   // processor output → frame
efp->AddCouple("genr", "transd", "out", "arriv"); // arrivals → transducer
efp->AddCouple("ef", "transd", "IN", "solved");   // completions → transducer
efp->AddCouple("transd", "genr", "out", "stop");  // end-of-window → stop generator
```

---

## 3. The Poisson arrival process

### 3.1 What a Poisson distribution means

The Poisson distribution models **how many independent events occur in a fixed
interval, given a known average rate**. It has a single parameter **λ (lambda)**,
the mean. The probability of exactly *k* events is:

```
P(k) = (λ^k · e^(-λ)) / k!
```

For λ = 3 (the default in this example) the draws cluster around 3 but vary:

| k | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
|---|-----|-----|-----|-----|-----|-----|-----|
| P(k) | 0.05 | 0.15 | 0.22 | 0.22 | 0.17 | 0.10 | 0.05 |

Poisson is the standard model for **arrival processes** — customers reaching a
queue, packets hitting a router, requests arriving at a server — because such
arrivals are irregular yet have a stable long-run average.

### 3.2 The generator class

The distribution is encapsulated in
[`PoissonRandomNumberGenerator`](../DEVS/include/PoissonRandomNumberGenerator.hpp),
a thin wrapper over the C++11 `<random>` facilities:

```cpp
class PoissonRandomNumberGenerator {
    std::random_device seed_gen;                 // hardware entropy for the seed
    std::default_random_engine engine;           // the RNG stream
    std::poisson_distribution<int> poisson;      // shapes randomness into Poisson
public:
    PoissonRandomNumberGenerator(double mean) : engine(seed_gen()), poisson(mean) {}
    int Generate();                              // one Poisson-distributed draw
};
```

### 3.3 How it plugs into DEVS

The connection between the distribution and the formalism is a single idea:
**a random draw fills `Sigma`.** In the `Generator`:

```cpp
void Generator::InitializeFN(void) {
    InterArrivalTime = arrivalGenerator.Generate();
    Count = 0;
    HoldIn("busy", 0.0);          // emit the first job immediately to prime the model
}

void Generator::IntTransitionFN(void) {
    if (Phase == "busy") {
        InterArrivalTime = arrivalGenerator.Generate();  // draw the next gap
        HoldIn("busy", InterArrivalTime);                // schedule the next arrival
    } else {
        Passivate();
    }
}
```

Every time the generator fires, it emits a job (`OutputFN`) and then draws a fresh
Poisson gap to schedule the following one. The DEVS engine does not care that the
duration is random — `Sigma` can be a constant, a formula, or a random variate.
This makes the model a **stochastic DEVS** model, the standard way to inject
real-world randomness into an event-driven simulation.

The mean is configurable through the constructor (default `3.0`):

```cpp
efp->AddItem(new Generator("genr", 5.0));   // λ = 5.0
```

---

## 4. Walkthrough of a simulation trace

Below is an actual run (the generator seeds from `std::random_device`, so each run
differs). The Poisson draws for the first few inter-arrival gaps were
`1, 3, 4, …`.

### 4.1 Priming step (t = 0)

```
genr(OUT) --> Phase: busy / Sigma: 0.000000 / When: 0.000000   ← emit Job-0 now
transd(EXT) --> :arriv:Job-0 at 0.000000                        ← transducer records arrival
genr(INT) --> Next inter-arrival time (Poisson, mean=3.0): 1    ← next gap drawn = 1
Global Clock (Root): 0.000000
```

The generator emits `Job-0` at time 0, then draws the next gap (1) and schedules
itself for t = 1.

### 4.2 Event-driven time advance

Notice how the global clock **jumps** to each scheduled event rather than
stepping uniformly:

```
Global Clock (Root): 0.000000
Global Clock (Root): 1.000000      ← Job-1 arrives (gap was 1)
Global Clock (Root): 4.000000      ← Job-2 arrives (gap was 3)
```

There is no computation between t = 1 and t = 4 — DEVS skips straight over the
empty interval. This is the essence of discrete-event simulation.

### 4.3 The summary after the 100-unit observation window

```
   ---------------------< Arrived Jobs >---------------------      ---------< Solved Jobs >---------
(Job-0, 0.000000)      gap                                         (Job-0, 7.000000)
(Job-1, 1.000000)       1                                          (Job-1, 14.000000)
(Job-2, 4.000000)       3                                          (Job-2, 21.000000)
(Job-3, 8.000000)       4                                          (Job-3, 28.000000)
(Job-4, 11.000000)      3                                          (Job-4, 35.000000)
(Job-5, 13.000000)      2                                          (Job-5, 42.000000)
(Job-6, 16.000000)      3                                          (Job-6, 49.000000)
(Job-7, 19.000000)      3                                          (Job-7, 56.000000)
(Job-8, 20.000000)      1                                          (Job-8, 63.000000)
(Job-9, 25.000000)      5                                          (Job-9, 70.000000)
```

Two things are visible:

1. **Arrival gaps vary** (1, 3, 4, 3, 2, 3, 3, 1, 5, …) yet average close to
   λ = 3. This is the Poisson process at work. When a draw is `0`, two jobs even
   share a timestamp — a "burst".
2. **Completions are perfectly regular** — every 7 units (7, 14, 21, 28, …),
   because the server's `PTime` is a fixed `7.0`.

---

## 5. What the example demonstrates

### 5.1 DEVS concepts

- **Event-driven time advance.** The global clock jumps between scheduled events;
  `Sigma` / `HoldIn` is the scheduling mechanism.
- **The atomic-model contract.** The `Generator` is a complete, self-contained
  DEVS component expressed solely through `InitializeFN`, `OutputFN`,
  `IntTransitionFN`, `ExtTransitionFN`, and `Sigma` — including a self-scheduling
  loop (each internal transition schedules the next).
- **Modularity via coupling.** The three components are wired only through ports.
  The Poisson generator can be swapped for another arrival model without touching
  the processor or transducer.
- **Stochastic DEVS.** Randomness enters the deterministic formalism simply by
  letting a random variate determine `Sigma`.

### 5.2 A queueing-theory result that emerges

The example is not just a mechanical demo; the numbers tell a physical story.

- Mean inter-arrival time ≈ **3** units → arrival rate λ ≈ **1/3** jobs/unit.
- Service time = **7** units → service rate μ = **1/7** jobs/unit.

Since arrivals (one every ~3 units) come **faster than** the server can clear them
(one every 7 units), the utilisation ρ = λ/μ ≈ 7/3 ≈ **2.3 > 1**. The queue is
**unstable**: it grows without bound. You can see this directly in the trace — the
gap between a job's arrival time and its solved time widens continuously
(`Job-0`: 0 → 7, a 7-unit wait; `Job-9`: 25 → 70, a 45-unit wait).

This is a genuine, observable consequence of the model, not something coded in —
exactly the kind of insight simulation is meant to reveal. Lowering the arrival
rate (raising λ, i.e. longer gaps) or shortening `PTime` would move the system
toward stability.

---

## 6. Building and running

From the [`DEVS`](../DEVS) directory:

```sh
cmake CMakeLists.txt && make && ./main.out
```

On Windows, open [`DEVS.sln`](../DEVS.sln) in Visual Studio and build the `DEVS`
project.

> **Reproducibility.** The generator seeds from `std::random_device`, so every run
> produces a different arrival sequence. Replace the seed with a fixed value in
> [`PoissonRandomNumberGenerator.hpp`](../DEVS/include/PoissonRandomNumberGenerator.hpp)
> for deterministic runs.

---

## 7. Summary

This example expresses a **random (Poisson) arrival process as a self-scheduling
DEVS atomic model**, wires it into a modular single-server queue, and lets the
event-driven kernel play the system forward in simulated time. It demonstrates the
two pillars of DEVS — event-driven behaviour (`Sigma`/`HoldIn`) and modular
coupling — while producing a meaningful queueing-theory outcome (an overloaded,
unstable server) that emerges from the interaction of a stochastic source and a
fixed-rate service.
