#ifndef __PROCESS_HPP__
#define __PROCESS_HPP__

#include <queue>
#include <string>

#include "../kernel/include/Log.hpp"
#include "../kernel/include/Atomic.hpp"

class Process : public Atomic {
public:
    std::string JobID;
	double PTime;
    std::queue<std::string> Queue;
public:
	Process(std::string entity_name);

    void ExtTransitionFN(double, DevsMessage);
	void IntTransitionFN(void);
	void OutputFN(void);
	void InitializeFN(void);
};

#endif	// __PROCESS_HPP__