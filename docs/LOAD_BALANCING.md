# Weighted Load Balancing in DEVS

This example extends the single-server queue ([`POISSON_DEVS_REPORT.md`](POISSON_DEVS_REPORT.md)) into a **weighted traffic distributor**: one dispatcher routes each arriving job to one of several backend servers according to configured weights. It is built entirely from the existing DEVS kernel — no kernel changes — and is compiled as a separate executable, `balancer.out`.

## Topology

```
                              ┌─(out0)─▶ Process-0 ─┐
  Generator ──▶ Balancer ─────┼─(out1)─▶ Process-1 ─┼──▶ Transducer
   (Poisson       (weighted   └─(out2)─▶ Process-2 ─┘      (solved)
    arrivals)      dispatch)
```

Arrivals are the same Poisson process as the base model (exponential inter-arrival times, mean 3). The `Balancer` replaces the single `Process`; three `Process` servers sit behind it, and every completion still flows to the `Transducer`. The wiring lives in [`src/balancer_main.cpp`](../DEVS/src/balancer_main.cpp).

## How routing works

A DEVS atomic model routes by **port name**: `MakeContent(port, value)` hands the message to the parent `Digraph`, which delivers it only along couplings whose source port matches. So the `Balancer` exposes one output port per server — `out0`, `out1`, `out2` — and emitting on `out{k}` reaches exactly `Process-k`:

```cpp
efp->AddCouple("Balancer", "Process-0", "out0", "in");
efp->AddCouple("Balancer", "Process-1", "out1", "in");
efp->AddCouple("Balancer", "Process-2", "out2", "in");
```

The target of each job is chosen by a **selector strategy**. Both implement the [`WeightedSelector`](../DEVS/include/WeightedSelector.hpp) interface (`int Generate()`), so the `Balancer` is agnostic to which one it holds:

| Strategy | Class | Behaviour |
|----------|-------|-----------|
| Weighted random *(default)* | [`WeightedRandomNumberGenerator`](../DEVS/include/WeightedRandomNumberGenerator.hpp) | `std::discrete_distribution<int>`; each job independently lands on server *i* with probability `weight[i] / Σweight`. Matches the weights *in expectation*. |
| Weighted round-robin | [`WeightedRoundRobinGenerator`](../DEVS/include/WeightedRoundRobinGenerator.hpp) | Deterministic *smooth* round-robin (the nginx algorithm); over each full cycle server *i* is chosen exactly `weight[i]` times, interleaved rather than in bursts. |

The strategy is chosen at construction:

```cpp
Balancer("Balancer", {5.0, 3.0, 2.0});                                       // weighted random
Balancer("Balancer", {5.0, 3.0, 2.0}, Balancer::Strategy::WeightedRoundRobin);
```

The [`Balancer`](../DEVS/src/Balancer.cpp) forwards with zero service delay: on each arrival it queues the job together with a target drawn from the selector, then emits it on the matching port at the same simulated instant (mirroring the queue handling of `Process`). It keeps a per-server tally so the split can be checked.

## Running it

```sh
cd DEVS
cmake CMakeLists.txt && make
./balancer.out        # weighted random (default)
./balancer.out rr     # weighted round-robin
```

Each routing decision is logged, e.g.:

```
Balancer(INT) --> Job-7 -> server 0 (routed so far: server0=1)
```

## Results (weights `5:3:2`)

**Weighted random** — one real 100-unit run routed 34 jobs:

| Server | Weight | Target share | Jobs routed | Actual share |
|--------|--------|--------------|-------------|--------------|
| Process-0 | 5 | 50% | 15 | 44% |
| Process-1 | 3 | 30% | 13 | 38% |
| Process-2 | 2 | 20% | 6  | 18% |

The shares approach the weights and converge as the sample grows; over a short window (~34 jobs) sampling variance is expected, and the numbers differ each run (the selector seeds from `std::random_device`).

**Weighted round-robin** — deterministic. Each 10-job cycle emits the exact interleaved sequence

```
server 0, 1, 2, 0, 0, 1, 0, 2, 1, 0     (5×server0, 3×server1, 2×server2)
```

so the routed counts track `5:3:2` precisely at every cycle boundary (e.g. 38 jobs → 19 / 11 / 8) — no variance, no bursts.

In both runs each `Process-k` processed exactly the number of jobs routed to it, confirming the port-based routing is correctly targeted.

## Scope and limitations

- Two stateless strategies are provided (weighted random, weighted round-robin). Adding another is just a new [`WeightedSelector`](../DEVS/include/WeightedSelector.hpp) implementation.
- **Not load-aware.** Least-connections / join-shortest-queue balancing would require feedback couplings from the servers back to the `Balancer` so it can see queue depths; that is a larger model.
- **Build:** `balancer.out` is defined in `CMakeLists.txt`. The Visual Studio project ([`DEVS.sln`](../DEVS.sln)) builds only the single-server `main.out`; use CMake for this demo.
