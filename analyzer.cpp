#include <iostream>
#include <vector>
#include "z3++.h"
#include "variable.hpp"
#include "ast.hpp"
#include "program.hpp"
#include "observer.hpp"
#include "noic3.hpp"
#include "analyzer.hpp"


using namespace z3;


std::vector<Variable*> filter_variables(std::vector<Variable*> vars, bool global){
	std::vector<Variable*> result;
	for(const auto v : vars){
		if(v->isglobal() == global){
			result.push_back(v);
		}
	}
	return result;
}




Analyzer::Analyzer(Program *prog, Observer *obs, context &ctxt):
	ctxt(ctxt),
	view1(prog, obs, ctxt, SingleVariable::get_nonobsvariables(), SingleVariable::get_obsvariables(), SingleVariable::get_variables(), filter_variables(SingleVariable::get_variables(), false), filter_variables(SingleVariable::get_variables(), true)),
	view2(prog, obs, ctxt, TwinVariable::get_nonobsvariables(), TwinVariable::get_obsvariables(), TwinVariable::get_variables(), filter_variables(SingleVariable::get_twins(1), false), SingleVariable::get_twins(2), filter_variables(SingleVariable::get_twins(2), false), SingleVariable::get_twins(1)),
	mediator(ctxt, view1.get_old_variables(), view2.get_old_variables())
{
}

void Analyzer::build_translator(){
	std::vector<Variable*> vars = SingleVariable::get_variables();
	std::vector<Variable*> obsvars = SingleVariable::get_obsvariables();
	int size;
	
	std::vector<int> v_single = view1.get_variables_single();
	std::vector<int> v1_single = view2.get_variables_single();
	std::vector<int> v2_single = view2.get_variables2_single();
	size = v_single.size();
	for(int i=0; i<size; i++){
		mediator.add(v_single[i], v1_single[i], v2_single[i]);
	}
	
	std::vector<Z3VariableContainer1D*> v_1did = view1.get_variables_1did();
	std::vector<Z3VariableContainer1D*> v1_1did = view2.get_variables_1did();
	std::vector<Z3VariableContainer1D*> v2_1did = view2.get_variables2_1did();
	size = v_1did.size();
	for(int i=0; i<size; i++){
		mediator.add_id(*v_1did[i], *v1_1did[i], *v2_1did[i]);
	}
	
	std::vector<Z3VariableContainer1D*> v_1dvar = view1.get_variables_1dvar();
	std::vector<Z3VariableContainer1D*> v1_1dvar = view2.get_variables_1dvar();
	std::vector<Z3VariableContainer1D*> v2_1dvar = view2.get_variables2_1dvar();
	size = v_1dvar.size();
	for(int i=0; i<size; i++){
		mediator.add_1dvar(*v_1dvar[i], *v1_1dvar[i], *v2_1dvar[i], vars);
	}
	
	std::vector<Z3VariableContainer1D*> v_1dobsvar = view1.get_variables_1dobsvar();
	std::vector<Z3VariableContainer1D*> v1_1dobsvar = view2.get_variables_1dobsvar();
	std::vector<Z3VariableContainer1D*> v2_1dobsvar = view2.get_variables2_1dobsvar();
	size = v_1dobsvar.size();
	for(int i=0; i<size; i++){
		mediator.add_1dvar(*v_1dobsvar[i], *v1_1dobsvar[i], *v2_1dobsvar[i], obsvars);
	}
	
	std::vector<Z3VariableContainer2D*> v_2dvar = view1.get_variables_2dvar();
	std::vector<Z3VariableContainer2D*> v1_2dvar = view2.get_variables_2dvar();
	std::vector<Z3VariableContainer2D*> v2_2dvar = view2.get_variables2_2dvar();
	size = v_2dvar.size();
	for(int i=0; i<size; i++){
		mediator.add_2dvar(*v_2dvar[i], *v1_2dvar[i], *v2_2dvar[i], vars);
	}
	
	std::vector<Z3VariableContainer2D*> v_2dnextvar = view1.get_variables_2dnextvar();
	std::vector<Z3VariableContainer2D*> v1_2dnextvar = view2.get_variables_2dnextvar();
	std::vector<Z3VariableContainer2D*> v2_2dnextvar = view2.get_variables2_2dnextvar();
	size = v_2dnextvar.size();
	for(int i=0; i<size; i++){
		mediator.add_2dnextvar(*v_2dnextvar[i], *v1_2dnextvar[i], *v2_2dnextvar[i], vars);
	}
	
	mediator.check_complete();
}

void Analyzer::prepare(){
	view1.build_initial();
	view1.build_transition();
	view1.build_safety();
	view2.build_transition();
	view2.build_intconstraints();
	build_translator();
}

bool Analyzer::analyze(){
	std::vector<expr> &seq_old_variables = view1.get_old_variables();
	std::vector<expr> &seq_new_variables = view1.get_new_variables();
	std::vector<expr> &int_old_variables = view2.get_old_variables();
	std::vector<expr> &int_new_variables = view2.get_new_variables();
	expr &transition = view1.get_transition();
	expr &inttransition = view2.get_transition();
	Frame &intconstraints = view2.get_intconstraints();
	Frame &initial = view1.get_initial();
	Frame &safety = view1.get_safety();
	NoIC3 ic3(ctxt, seq_old_variables, seq_new_variables, int_old_variables, int_new_variables, mediator, initial, transition, inttransition, intconstraints, safety);
	return ic3.prove();
}




