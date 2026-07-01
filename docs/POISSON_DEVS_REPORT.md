# A Poisson Arrival Process in DEVS — Model Report

This report explains the example added in this repository: a single-server queueing model built on the **DEVS** formalism, whose job arrivals form a **Poisson process** (exponentially distributed inter-arrival times). It covers the formalism, the model structure, the arrival generator, a walkthrough of an actual simulation trace, and what the results demonstrate.

---

## 1. Background: what DEVS is and why it matters here

**DEVS (Discrete Event System Specification)** is a formalism, introduced by Bernard Zeigler, for modelling systems whose state changes only at discrete instants in time. Its defining characteristic is that simulation is **event-driven, not clock-driven**: rather than advancing time in fixed steps and asking "did anything change?", a DEVS simulator computes *when the next event will occur* and jumps the global clock straight to that instant.

This matters for the present example because job arrivals happen at irregular, random moments. A fixed-step simulator would waste effort ticking through empty intervals; DEVS advances directly from one arrival (or service completion) to the next.

A DEVS model is built from two kinds of components:

- **Atomic models** — indivisible behavioural units. Each is defined by exactly four functions and one state variable, `Sigma` (the time remaining until its next internal event).
- **Coupled models** — structures that compose atomic (and other coupled) models by connecting their input and output **ports**.

### The atomic-model contract

| Function | Trigger | Responsibility |
|----------|---------|----------------|
| `InitializeFN` | simulation start | set initial phase/state and first `Sigma` |
| `OutputFN` | just **before** an internal transition | emit output messages |
| `IntTransitionFN` | `Sigma` elapses (internal event) | update state, schedule next `Sigma` |
| `ExtTransitionFN` | an input message arrives (external event) | react to the event |

The key mechanism is **`HoldIn(phase, duration)`**, which sets the phase and schedules the next internal event `duration` time units into the future (`Sigma = duration`). `Passivate()` means "wait indefinitely" (`Sigma = ∞`), and `Continue()` keeps the current schedule.

---

## 2. The model: the `ef-p` single-server queue

The example builds the classic **experimental-frame / processor** (`ef-p`) model. It is the "hello world" of queueing simulation.

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
| **Generator** (`genr`) | [`src/Generator.cpp`](../DEVS/src/Generator.cpp) | Emits jobs `Job-0`, `Job-1`, …. The gap between successive jobs is an exponential draw (a Poisson arrival process). |
| **Process** | [`src/Process.cpp`](../DEVS/src/Process.cpp) | A single-server queue. Buffers jobs and serves them one at a time, each taking `PTime = 7.0` time units. |
| **Transducer** (`transd`) | [`src/Transducer.cpp`](../DEVS/src/Transducer.cpp) | Observes arrivals and completions for a window of `100.0` units, then prints a summary. |

The wiring lives in [`src/main.cpp`](../DEVS/src/main.cpp) and is expressed purely as port couplings — the components have no direct knowledge of one another:

```cpp
efp->AddCouple("ef", "Process", "OUT", "in");   // frame → processor input
efp->AddCouple("Process", "ef", "out", "IN");   // processor output → frame
efp->AddCouple("genr", "transd", "out", "arriv"); // arrivals → transducer
efp->AddCouple("ef", "transd", "IN", "solved");   // completions → transducer
efp->AddCouple("transd", "genr", "out", "stop");  // end-of-window → stop generator
```

---

## 3. The Poisson arrival process

### 3.1 Two equivalent views of a Poisson process

A **Poisson process** is the canonical model for random arrivals — customers reaching a queue, packets hitting a router, requests arriving at a server: events that occur independently at a constant average rate **λ**. It can be described two equivalent ways:

- **Count view.** The *number* of arrivals in a fixed interval of length *t* follows a **Poisson distribution** with mean λ·t:  `P(k) = ((λt)^k · e^(-λt)) / k!`.
- **Timing view.** The *time between* consecutive arrivals follows an **exponential distribution** with rate λ (mean `1/λ`).

Both describe the same process. A discrete-event simulation needs the timing view: to schedule the next arrival it must draw an inter-arrival *time*, so it samples the **exponential** distribution. (Drawing the gap directly from a Poisson distribution would be a different, non-Poisson renewal process, and would even permit zero-length gaps.)

This repository provides both distributions as small `<random>` wrappers — the simulation uses the exponential one; the Poisson wrapper is included as the count-view counterpart (reference only, not compiled into the executable):

| Wrapper | Distribution | Role |
|---|---|---|
| [`ExponentialRandomNumberGenerator`](../DEVS/include/ExponentialRandomNumberGenerator.hpp) | `std::exponential_distribution<double>` | inter-arrival **times** — drives the simulation |
| [`PoissonRandomNumberGenerator`](../DEVS/include/PoissonRandomNumberGenerator.hpp) | `std::poisson_distribution<int>` | the equivalent arrival **count** per interval |

### 3.2 The arrival generator

```cpp
class ExponentialRandomNumberGenerator {
    std::random_device seed_gen;                        // hardware entropy for the seed
    std::default_random_engine engine;                  // the RNG stream
    std::exponential_distribution<double> exponential;  // rate lambda = 1 / mean
public:
    ExponentialRandomNumberGenerator(double mean)
        : engine(seed_gen()), exponential(1.0 / mean) {}
    double Generate();                                  // one inter-arrival time
};
```

With `mean = 3.0` the rate is λ = 1/3, i.e. on average one arrival every 3 time units.

### 3.3 How it plugs into DEVS

The connection between the distribution and the formalism is a single idea: **a random draw fills `Sigma`.** In the `Generator`:

```cpp
void Generator::InitializeFN(void) {
    Count = 0;
    HoldIn("busy", 0.0);   // emit the first job immediately; IntTransitionFN draws the next gap
}

void Generator::IntTransitionFN(void) {
    if (Phase == "busy") {
        InterArrivalTime = arrivalGenerator.Generate();  // draw the next inter-arrival time
        HoldIn("busy", InterArrivalTime);                // schedule the next arrival
    } else {
        Passivate();
    }
}
```

Every internal transition emits a job (`OutputFN`) and then draws a fresh exponential gap to schedule the following one. The DEVS engine does not care that the duration is random — `Sigma` can be a constant, a formula, or a random variate. This makes the model a **stochastic DEVS** model, the standard way to inject real-world randomness into an event-driven simulation. The mean is set by the `ARRIVAL_MEAN` constant in `Generator.cpp` (default `3.0`).

---

## 4. Walkthrough of a simulation trace

The excerpts below are from one real run. The generator seeds from `std::random_device`, so the exact numbers differ every run — treat them as illustrative.

### 4.1 Priming step (t = 0)

```
genr(OUT) --> Phase: busy / Sigma: 0.000000 / When: 0.000000
transd(EXT) --> :arriv:Job-0 at 0.000000
genr(INT) --> Next inter-arrival time (exp, mean=3.000000): 6.178253
Global Clock (Root): 0.000000
```

The generator emits `Job-0` at time 0, then draws the next inter-arrival time (6.178253) and schedules itself for that instant.

### 4.2 Event-driven time advance

Notice how the global clock **jumps** to each scheduled event — arrivals (exponential, irregular) interleaved with completions (every 7 units) — rather than stepping uniformly:

```
Global Clock (Root): 0.000000
Global Clock (Root): 6.178253      ← Job-1 arrives
Global Clock (Root): 7.000000      ← Process finishes Job-0
Global Clock (Root): 9.817465      ← Job-2 arrives
Global Clock (Root): 14.000000     ← Process finishes Job-1
```

There is no computation between these instants — DEVS skips straight over the empty intervals. This is the essence of discrete-event simulation.

### 4.3 The summary after the 100-unit observation window

```
   ------< Arrived Jobs >------          ------< Solved Jobs >------
(Job-0,  0.000000)                       (Job-0,  7.000000)
(Job-1,  6.178253)                       (Job-1, 14.000000)
(Job-2,  9.817465)                       (Job-2, 21.000000)
(Job-3, 16.470632)                       (Job-3, 28.000000)
(Job-4, 23.040444)                       (Job-4, 35.000000)
(Job-5, 25.212080)                       (Job-5, 42.000000)
 ...                                      ...
(Job-35, 98.455765)                      (Job-13, 98.000000)
```

Two things are visible:

1. **Arrival timestamps are irregular** (gaps 6.18, 3.64, 6.65, 2.17, …), the signature of an exponential/Poisson process — and, being continuous, never coincide.
2. **Completions are perfectly regular** — every 7 units (7, 14, 21, …), because the server's `PTime` is a fixed `7.0`.

In this window **36 jobs arrived but only 14 were served** — the queue is falling behind (see §5.2).

---

## 5. What the example demonstrates

### 5.1 DEVS concepts

- **Event-driven time advance.** The global clock jumps between scheduled events; `Sigma` / `HoldIn` is the scheduling mechanism.
- **The atomic-model contract.** The `Generator` is a complete, self-contained DEVS component expressed solely through `InitializeFN`, `OutputFN`, `IntTransitionFN`, `ExtTransitionFN`, and `Sigma` — including a self-scheduling loop (each internal transition schedules the next).
- **Modularity via coupling.** The three components are wired only through ports. The arrival generator can be swapped for another arrival model without touching the processor or transducer.
- **Stochastic DEVS.** Randomness enters the deterministic formalism simply by letting a random variate determine `Sigma`.

### 5.2 A queueing-theory result that emerges

The example is not just a mechanical demo; the numbers tell a physical story.

- Arrival rate λ = **1/3** jobs/unit (mean inter-arrival time `1/λ` = **3** units).
- Service rate μ = **1/7** jobs/unit (service time = **7** units).

Since arrivals come **faster than** the server can clear them, the utilisation ρ = λ/μ = 7/3 ≈ **2.3 > 1**. The queue is **unstable**: it grows without bound. The run above shows this directly — over the 100-unit window **36 jobs arrived but only 14 completed**, so the backlog keeps growing and each job waits longer than the last.

This is a genuine, observable consequence of the model, not something coded in — exactly the kind of insight simulation is meant to reveal. Lengthening the mean inter-arrival time (`ARRIVAL_MEAN`) past the 7-unit service time, or shortening `PTime`, would move the system toward stability (ρ < 1).

---

## 6. Building and running

From the [`DEVS`](../DEVS) directory:

```sh
cmake CMakeLists.txt && make && ./main.out
```

On Windows, open [`DEVS.sln`](../DEVS.sln) in Visual Studio and build the `DEVS` project.

> **Reproducibility.** The generator seeds from `std::random_device`, so every run produces a different arrival sequence. Replace the seed with a fixed value in [`ExponentialRandomNumberGenerator.hpp`](../DEVS/include/ExponentialRandomNumberGenerator.hpp) for deterministic runs.

---

## 7. Summary

This example expresses a **random (Poisson) arrival process as a self-scheduling DEVS atomic model**, wires it into a modular single-server queue, and lets the event-driven kernel play the system forward in simulated time. It demonstrates the two pillars of DEVS — event-driven behaviour (`Sigma`/`HoldIn`) and modular coupling — while producing a meaningful queueing-theory outcome (an overloaded, unstable server) that emerges from the interaction of a stochastic source and a fixed-rate service.
