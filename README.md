# DEVS: Discrete Event System Specification

A minimal C++ DEVS implementation with a single-server queueing example whose job arrivals follow a **Poisson distribution**. See [`docs/POISSON_DEVS_REPORT.md`](docs/POISSON_DEVS_REPORT.md) for the model and details.

## Build

```
$ brew install cmake
$ cd DEVS
$ cmake CMakeLists.txt
$ make
$ ./main.out
```
