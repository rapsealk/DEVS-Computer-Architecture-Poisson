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

The target of each job is chosen by [`WeightedRandomNumberGenerator`](../DEVS/include/WeightedRandomNumberGenerator.hpp), a thin wrapper over `std::discrete_distribution<int>` (the same pattern as the exponential arrival generator):

```cpp
WeightedRandomNumberGenerator picker({5.0, 3.0, 2.0});  // 50% / 30% / 20%
int server = picker.Generate();                          // 0, 1, or 2
```

The [`Balancer`](../DEVS/src/Balancer.cpp) forwards with zero service delay: on each arrival it queues the job together with a freshly drawn target, then emits it on the matching port at the same simulated instant (mirroring the queue handling of `Process`). It keeps a per-server tally so the split can be checked.

## Running it

```sh
cd DEVS
cmake CMakeLists.txt && make
./balancer.out
```

Each routing decision is logged, e.g.:

```
Balancer(INT) --> Job-7 -> server 0 (routed so far: server0=1)
```

## Result

One real 100-unit run (weights `5:3:2`) routed **34 jobs**:

| Server | Weight | Target share | Jobs routed | Actual share |
|--------|--------|--------------|-------------|--------------|
| Process-0 | 5 | 50% | 15 | 44% |
| Process-1 | 3 | 30% | 13 | 38% |
| Process-2 | 2 | 20% | 6  | 18% |

Each `Process-k` processed exactly the number of jobs routed to it (15 / 13 / 6), confirming the port-based routing is correctly targeted. The observed shares approach the configured weights and converge as the sample grows; over a short window (~34 jobs) sampling variance is expected. The generator seeds from `std::random_device`, so numbers differ each run.

## Scope and limitations

- **Weighting is stateless weighted-random.** For deterministic *weighted round-robin*, replace the random pick with a weighted counter in the `Balancer` — same model, same ports.
- **Not load-aware.** Least-connections / join-shortest-queue balancing would require feedback couplings from the servers back to the `Balancer` so it can see queue depths; that is a larger model.
- **Build:** `balancer.out` is defined in `CMakeLists.txt`. The Visual Studio project ([`DEVS.sln`](../DEVS.sln)) builds only the single-server `main.out`; use CMake for this demo.
