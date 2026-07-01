#ifndef __GENERATOR_HPP__
#define __GENERATOR_HPP__

#include "../kernel/include/Log.hpp"
#include "../kernel/include/Atomic.hpp"

#include "ExponentialRandomNumberGenerator.hpp"

class Generator : public Atomic {
public:
	double  InterArrivalTime;
//	int     ProcessingTime;
//	int     ProblemLevel;
	int     Count;

private:
	ExponentialRandomNumberGenerator arrivalGenerator;

public:
	Generator();
	Generator(std::string);

    void ExtTransitionFN(double,DevsMessage);
	void IntTransitionFN(void);
	void OutputFN(void);
	void InitializeFN(void);
};

#endif	// __GENERATOR_HPP__