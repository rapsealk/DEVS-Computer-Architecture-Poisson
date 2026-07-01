#ifndef __BALANCER_HPP__
#define __BALANCER_HPP__

#include <initializer_list>

#include "../kernel/include/Log.hpp"
#include "../kernel/include/Atomic.hpp"

#include "WeightedRandomNumberGenerator.hpp"

// Forwards each incoming job to one of N servers, chosen by weight.
// Emits on ports "out0" .. "out{N-1}" with zero delay.
class Balancer : public Atomic {
public:
	std::string Queue[100];
	int         Route[100];   // target server for each queued job
	int         Front, Tail;

	int         NumServers;
	int         RouteCount[16];   // per-server tally, for verification

private:
	WeightedRandomNumberGenerator picker;

public:
	Balancer(std::string, std::initializer_list<double>);

	void ExtTransitionFN(double, DevsMessage);
	void IntTransitionFN(void);
	void OutputFN(void);
	void InitializeFN(void);
};

#endif	// __BALANCER_HPP__
