#include "../include/Generator.hpp"

#include <iostream>
#include <random>

static const double ARRIVAL_MEAN = 3.0;   // Poisson mean (lambda)

Generator::Generator()
	: Generator::Generator("Generator") { }

Generator::Generator(std::string entity_name)
	: Atomic(entity_name), arrivalGenerator(ARRIVAL_MEAN) {
	SetName(entity_name);
}

void Generator::ExtTransitionFN(double E, DevsMessage X) {
	Logln(Name + "(EXT) --> :" + X.ContentPort() + ": " + "When: " + std::to_string(AddTime(GetLastEventTime(), E)));
	
	if (X.ContentPort() == "stop") Passivate();
}

void Generator::IntTransitionFN(void) {
	Logln(Name + "(INT) --> Sigma: " + std::to_string(Sigma) + " / When: " + std::to_string(AddTime(GetLastEventTime(), Sigma)));
	if (Phase == "busy") {
		InterArrivalTime = arrivalGenerator.Generate();
		Logln(Name + "(INT) --> Next inter-arrival time (Poisson, mean=" + std::to_string(ARRIVAL_MEAN) + "): " + std::to_string(InterArrivalTime));
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
	InterArrivalTime = arrivalGenerator.Generate();
	Count = 0;

	HoldIn("busy", 0.0);   // emit the first job immediately
}