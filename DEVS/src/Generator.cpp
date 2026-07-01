#include "../include/Generator.hpp"

#include <iostream>
#include <random>

// Default mean (lambda) of the Poisson arrival process, in time units.
static const double DEFAULT_ARRIVAL_MEAN = 3.0;

Generator::Generator()
	: Generator::Generator("Generator") { }

Generator::Generator(std::string entity_name)
	: Generator::Generator(entity_name, DEFAULT_ARRIVAL_MEAN) { }

Generator::Generator(std::string entity_name, double arrival_mean)
	: Atomic(entity_name), ArrivalMean(arrival_mean), arrivalGenerator(arrival_mean) {
	SetName(entity_name);
}

void Generator::ExtTransitionFN(double E, DevsMessage X) {
	Logln(Name + "(EXT) --> :" + X.ContentPort() + ": " + "When: " + std::to_string(AddTime(GetLastEventTime(), E)));
	
	if (X.ContentPort() == "stop") Passivate();
}

void Generator::IntTransitionFN(void) {
	Logln(Name + "(INT) --> Sigma: " + std::to_string(Sigma) + " / When: " + std::to_string(AddTime(GetLastEventTime(), Sigma)));
	if (Phase == "busy") {
		// Draw the next inter-arrival time from the Poisson distribution.
		InterArrivalTime = arrivalGenerator.Generate();
		Logln(Name + "(INT) --> Next inter-arrival time (Poisson, mean=" + std::to_string(ArrivalMean) + "): " + std::to_string(InterArrivalTime));
		HoldIn("busy", InterArrivalTime);
	} else {
		Passivate();
	}
}

void Generator::OutputFN(void) {
	Logln(Name + "(OUT) --> Phase: " + Phase + " / Sigma: " + std::to_string(Sigma) + " / When: " + std::to_string(GetNextEventTime()));

	if (Phase == "busy") {
		MakeContent("out", "Job-" + std::to_string(Count++));
	}
	else MakeContent();
}

void Generator::InitializeFN(void) {
	// The first inter-arrival time is also drawn from the Poisson distribution,
	// but the very first job is emitted immediately (Sigma = 0) to prime the model.
	InterArrivalTime = arrivalGenerator.Generate();
	Count = 0;

	HoldIn("busy", 0.0);
}