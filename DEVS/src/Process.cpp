#include "../include/Process.hpp"
#include "../include/Utility.hpp"

#include <string>

Process::Process(std::string entity_name) : Atomic(entity_name) { 
    SetName(entity_name); 
}

void Process::ExtTransitionFN(double Time, DevsMessage MSG) {
	Log(Name + "(EXT) --> ");
	if (MSG.ContentPort() == "in") {

		// Put job into the Queue (unbounded, so a growing backlog is never dropped)
		Queue.push(MSG.ContentValue());
		Log(MSG.ContentPort() + ":" + JobID);

		if (Phase == "busy"){
			Continue();
		}
		else
		{
			if (!Queue.empty())
				HoldIn("busy",0.0);
		}
	}
	else Continue();
	Logln();
}

void Process::IntTransitionFN(void) {
	Log(Name + "(INT) --> ");
	if (Phase == "busy"){
		// Get job from the Queue
		if(!Queue.empty())
		{
			// processing
			JobID = Queue.front();
			Queue.pop();
			Log(" process : " + JobID);
			HoldIn("busy", PTime);
		}
		else
			Passivate();
	}
	else Continue();
	Logln();
}

void Process::OutputFN(void) {
	Log(Name + "(OUT) --> ");
	
	if (Phase == "busy"){ 
		MakeContent("out", JobID);
	}
	else MakeContent();
	Logln();
}

void Process::InitializeFN(void){
	PTime = (double) 7.0;
	Passivate();
	ClearMessageQueue(Queue);
}