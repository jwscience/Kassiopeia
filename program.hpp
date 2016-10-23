#pragma once

#include <string>
#include <iostream>
#include <vector>
#include "variable.hpp"
#include "ast.hpp"



class Function;
class Program;



class Function{
	private:
	std::string name;
	bool input_present;
	Expression *entrypoint;
	
	public:
	Function(std::string name, bool has_input, Sequence *seq);
	Expression* get_entrypoint();
	bool has_input();
	void printsubprogram();
};


class Program{
	private:
	std::string name;
	std::vector<Variable*> variables;
	Function *init;
	std::vector<Function*> functions;
	std::vector<Expression*> code;
	
	public:
	Program(std::string name, std::vector<Variable*> variables, Function *init, std::vector<Function*> functions);
	~Program();
	Function* get_init();
	std::vector<Function*> get_functions();
	std::vector<Expression*> get_code();
};





