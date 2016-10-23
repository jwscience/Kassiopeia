#pragma once

#include <vector>
#include "z3++.h"
#include "program.hpp"
#include "view.hpp"
#include "observer.hpp"
#include "noic3.hpp"
#include "z3variablecontainer.hpp"


using namespace z3;


class Analyzer;




class Analyzer{
	private:
	context &ctxt;
	OneThreadView view1;
	TwoThreadView view2;
	Mediator mediator;
	
	
	void build_translator();
	
	public:
	Analyzer(Program *prog, Observer *obs, context &ctxt);
	void prepare();
	bool analyze();
};






