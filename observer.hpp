#pragma once

#include <vector>
#include "variable.hpp"


class LinearisationEvent;
class Observer;


enum LinearisationType {IN, IN_PCMP, IN_PACMP, OUT, OUT_PCMP, OUT_PACMP};

class LinearisationEvent{
	private:
	LinearisationType type;
	Variable *var;
	Variable *cmp1;
	Variable *cmp2;
	
	public:
	LinearisationEvent(LinearisationType type, Variable *var);
	LinearisationEvent(LinearisationType type, Variable *var, Variable *cmp1, Variable *cmp2);
	LinearisationType gettype();
	Variable *getvar();
	Variable *getcmp1();
	Variable *getcmp2();
	void print();
};


/*
State 0 is always the initial state,
the last state is alway the accepting state.
*/
class Observer{
	private:
	std::vector<Variable*> variables;
	std::vector<std::vector<int>> intransitions;
	std::vector<std::vector<int>> outtransitions;
	
	public:
	Observer(std::vector<Variable*> variables, int states);
	~Observer();
	void add_transition(int source, int target, LinearisationType eventtype, Variable *var);
	int transition(int source, LinearisationEvent *event, Variable *var);
	int count_states();
};







