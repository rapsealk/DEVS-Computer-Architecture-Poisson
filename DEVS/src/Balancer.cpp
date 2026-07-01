#include "../include/Balancer.hpp"

#include "../include/WeightedRandomNumberGenerator.hpp"
#include "../include/WeightedRoundRobinGenerator.hpp"

Balancer::Balancer(std::string entity_name, std::initializer_list<double> weights, Strategy strategy)
	: Atomic(entity_name), NumServers((int) weights.size()),
	  selector(strategy == Strategy::WeightedRoundRobin
	           ? (WeightedSelector*) new WeightedRoundRobinGenerator(weights)
	           : (WeightedSelector*) new WeightedRandomNumberGenerator(weights)) {
	SetName(entity_name);
}

Balancer::~Balancer() {
	delete selector;
}

void Balancer::ExtTransitionFN(double E, DevsMessage X) {
	if (X.ContentPort() == "in") {
		if (Tail < 100) {
			Queue[Tail] = X.ContentValue();
			Route[Tail] = selector->Generate();   // decide the target on arrival
			Tail++;
		}
		else Logln(Name + "(EXT) --> queue full (100), dropping " + X.ContentValue());
		if (Phase == "busy") Continue();
		else if (Front != Tail) HoldIn("busy", 0.0);
	}
	else Continue();
}

void Balancer::OutputFN(void) {
	if (Phase == "busy" && Front != Tail)
		MakeContent("out" + std::to_string(Route[Front]), Queue[Front]);
	else MakeContent();
}

void Balancer::IntTransitionFN(void) {
	if (Phase == "busy") {
		if (Front != Tail) {
			int k = Route[Front];
			RouteCount[k]++;
			Logln(Name + "(INT) --> " + Queue[Front] + " -> server " + std::to_string(k)
				+ " (routed so far: server" + std::to_string(k) + "=" + std::to_string(RouteCount[k]) + ")");
			Front++;
		}
		if (Front != Tail) {
			HoldIn("busy", 0.0);   // drain the rest at this instant
		} else {
			Front = Tail = 0;      // queue emptied; reset the linear buffer so it stays bounded
			Passivate();
		}
	}
	else Continue();
}

void Balancer::InitializeFN(void) {
	Front = Tail = 0;
	RouteCount.assign(NumServers, 0);
	Passivate();
}
