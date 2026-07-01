# DEVS: Discrete Event System Specification

A small C++ implementation of the **DEVS** formalism together with a classic
queueing example whose job arrivals are driven by a **Poisson distribution**.

> For a detailed write-up of the formalism, the model, the Poisson generator, an
> annotated simulation trace, and what the results demonstrate, see
> [`docs/POISSON_DEVS_REPORT.md`](docs/POISSON_DEVS_REPORT.md).

## What is DEVS?

DEVS (Discrete Event System Specification) is a formalism for modelling systems
whose state changes at discrete points in time. A model is built from two kinds
of components:

- **Atomic models** — the leaves of the model tree. Each one defines four
  functions:
  - `InitializeFN` — set the initial state,
  - `ExtTransitionFN` — react to an incoming message (external event),
  - `IntTransitionFN` — react to an internal timeout (`Sigma` elapsing),
  - `OutputFN` — emit messages just before an internal transition.
- **Coupled models** — compose atomic (and other coupled) models by wiring
  their ports together.

The kernel that runs the simulation lives under [`DEVS/kernel`](DEVS/kernel).

## The example model (`ef-p`)

The example in [`DEVS/src/main.cpp`](DEVS/src/main.cpp) builds the well-known
**experimental-frame / processor** (`ef-p`) model:

```
                 ef  (experimental frame)
        ┌────────────────────────────────────┐
        │   ┌───────────┐      ┌───────────┐  │
  OUT ◀─┼── │ Generator │──────│Transducer │  │
        │   │  (genr)   │ arriv│ (transd)  │  │
        │   └───────────┘      └───────────┘  │
        │         │ out            ▲ solved   │
        └─────────┼─────────────── │ ─────────┘
             IN   ▼                │ OUT
        ┌────────────────────────────────────┐
        │            Process                  │
        │           (processor)               │
        └────────────────────────────────────┘
```

| Component | File | Role |
|-----------|------|------|
| **Generator** | [`Generator.cpp`](DEVS/src/Generator.cpp) | Emits jobs (`Job-0`, `Job-1`, …). The time between jobs is drawn from a Poisson distribution. |
| **Process** | [`Process.cpp`](DEVS/src/Process.cpp) | A single-server queue. Buffers incoming jobs and processes them one at a time (`PTime = 7.0` time units each). |
| **Transducer** | [`Transducer.cpp`](DEVS/src/Transducer.cpp) | Observes arrivals and completions, then prints a summary after an observation window of `100.0` time units. |

## The Poisson arrival process

Real-world arrival processes (customers, packets, requests) are commonly
modelled as a **Poisson process**. Instead of the fixed inter-arrival time the
original generator used, the `Generator` now draws each inter-arrival time from
a Poisson distribution.

The distribution is encapsulated in
[`PoissonRandomNumberGenerator`](DEVS/include/PoissonRandomNumberGenerator.hpp),
a thin wrapper over the C++11 `<random>` facilities:

```cpp
PoissonRandomNumberGenerator arrivals(3.0);  // mean (lambda) = 3.0
int gap = arrivals.Generate();               // e.g. 3, 4, 2, 0, 5, ...
```

Inside the `Generator`:

- `InitializeFN` draws the first inter-arrival time and emits the first job
  immediately (`Sigma = 0`) to prime the model.
- `IntTransitionFN` draws a fresh inter-arrival time from the distribution
  after every job and holds in the `busy` phase for that long.

The mean arrival rate (`lambda`) defaults to `3.0` and can be overridden through
the constructor:

```cpp
efp->AddItem(new Generator("genr", 5.0));  // lambda = 5.0
```

Because a Poisson draw can be `0`, two (or more) jobs may arrive at the same
simulated time — you will see this in the output as several arrivals sharing a
timestamp, which is exactly the "bursty" behaviour a Poisson process produces.

> **Note:** the generator seeds itself from `std::random_device`, so each run
> produces a different arrival sequence. Swap the seed for a fixed value in
> [`PoissonRandomNumberGenerator.hpp`](DEVS/include/PoissonRandomNumberGenerator.hpp)
> if you need reproducible runs.

## Build

The build is driven by CMake from the [`DEVS`](DEVS) directory:

```sh
brew install cmake        # macOS; use your package manager elsewhere
cd DEVS
cmake CMakeLists.txt
make
```

This produces an executable named `main.out`.

If CMake is not available you can compile directly with `g++` (C++11):

```sh
cd DEVS
g++ -std=c++11 -I kernel/include -I include \
    src/*.cpp kernel/src/*.cpp -o main.out
```

On Windows, open [`DEVS.sln`](DEVS.sln) in Visual Studio and build the `DEVS`
project.

## Run

```sh
./main.out
```

The simulation logs every transition. Look for the generator's Poisson draws:

```
genr(INT) --> Next inter-arrival time (Poisson, mean=3.000000): 4
```

and, at the end of the observation window, the transducer's summary of arrived
and solved jobs with their timestamps:

```
   ---------------------< Arrived Jobs >---------------------
(Job-0, 0.000000)
(Job-1, 3.000000)
(Job-2, 7.000000)
...
   ---------------------< Solved Jobs >---------------------
(Job-0, 7.000000)
(Job-1, 14.000000)
...
```

Comparing the arrival timestamps shows the varying, occasionally simultaneous
inter-arrival gaps characteristic of a Poisson process.
