# DEVS: Discrete Event System Specification

A minimal C++ DEVS implementation with a single-server queueing example whose job arrivals form a **Poisson process** (exponentially distributed inter-arrival times). See [`docs/POISSON_DEVS_REPORT.md`](docs/POISSON_DEVS_REPORT.md) for the model and details.

A second example distributes traffic across several servers by weight — see [`docs/LOAD_BALANCING.md`](docs/LOAD_BALANCING.md).

## Build

```
$ brew install cmake
$ cd DEVS
$ cmake CMakeLists.txt
$ make
$ ./main.out        # single-server Poisson queue
$ ./balancer.out    # weighted load-balancing across 3 servers
```
