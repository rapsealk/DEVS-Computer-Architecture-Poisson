#include "../include/Balancer.hpp"

Balancer::Balancer(std::string entity_name, std::initializer_list<double> weights)
	: Atomic(entity_name), NumServers((int) weights.size()), picker(weights) {
	SetName(entity_name);
}

void Balancer::ExtTransitionFN(double E, DevsMessage X) {
	if (X.ContentPort() == "in") {
		if (Tail < 100) {
			Queue[Tail] = X.ContentValue();
			Route[Tail] = picker.Generate();   // decide the target on arrival
			Tail++;
		}
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
			if (Front != Tail) HoldIn("busy", 0.0);   // drain the rest at this instant
			else Passivate();
		}
		else Passivate();
	}
	else Continue();
}

void Balancer::InitializeFN(void) {
	Front = Tail = 0;
	for (int i = 0; i < NumServers; i++) RouteCount[i] = 0;
	Passivate();
}
