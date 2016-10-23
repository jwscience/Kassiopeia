#pragma once


#include <vector>
#include "z3++.h"
#include "variable.hpp"


using namespace z3;



class Z3VariableContainer1D;
class Z3VariableContainer2D;




class Z3VariableContainer1D{
	private:
	std::vector<expr> variables;
	std::vector<int> ids;
	
	public:
	Z3VariableContainer1D(context &ctxt, int size, std::string basename, std::vector<expr> &globallist, std::vector<Variable*> *varlist=NULL);
	int size();
	expr operator() (unsigned int index);
	int getid(unsigned int index);
};

class Z3VariableContainer2D{
	private:
	std::vector<std::vector<expr>> variables;
	std::vector<std::vector<int>> ids;
	
	public:
	Z3VariableContainer2D(context &ctxt, int size, std::string basename, bool symmetric, std::vector<expr> &globallist, std::vector<Variable*> *varlist=NULL);
	int size();
	expr operator() (unsigned int index1, unsigned int index2);
	int getid(unsigned int index1, unsigned int index2);
};



