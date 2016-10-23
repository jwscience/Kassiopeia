#pragma once

#include <vector>
#include "z3++.h"
#include "variable.hpp"
#include "program.hpp"
#include "observer.hpp"
#include "z3variablecontainer.hpp"
#include "noic3.hpp"



using namespace z3;



class View;
class OneThreadView;
class TwoThreadView;





class View{
	protected:
	Program *program;
	Observer *observer;
	context &ctxt;
	std::vector<Variable*> nonobsvars;
	std::vector<Variable*> obsvars;
	std::vector<Variable*> vars;
	std::vector<Variable*> activelocalscope;
	std::vector<Variable*> activenotlocalscope;
	
	int loc;
	const int autc;
	const int varc;
	const int obsc;
	std::vector<expr> old_variables;
	std::vector<expr> new_variables;
	
	Z3VariableContainer1D pc;
	Z3VariableContainer1D os;
	Z3VariableContainer2D shape_eq;
	Z3VariableContainer2D shape_neq;
	Z3VariableContainer2D shape_next;
	Z3VariableContainer2D shape_reach;
	Z3VariableContainer2D ages_eq;
	Z3VariableContainer2D ages_lt;
	Z3VariableContainer1D own;
	Z3VariableContainer1D freed;
	Z3VariableContainer1D freednext;
	Z3VariableContainer1D in;
	Z3VariableContainer1D out;
	Z3VariableContainer1D inseen;
	Z3VariableContainer1D inv_eq;
	Z3VariableContainer1D inv_next;
	Z3VariableContainer1D inv_reach;
	Z3VariableContainer1D sin_eq;
	Z3VariableContainer1D sin_next;
	Z3VariableContainer1D sin_reach;
	expr sinout;
	int sinoutid;
	
	Z3VariableContainer1D Xpc;
	Z3VariableContainer1D Xos;
	Z3VariableContainer2D Xshape_eq;
	Z3VariableContainer2D Xshape_neq;
	Z3VariableContainer2D Xshape_next;
	Z3VariableContainer2D Xshape_reach;
	Z3VariableContainer2D Xages_eq;
	Z3VariableContainer2D Xages_lt;
	Z3VariableContainer1D Xown;
	Z3VariableContainer1D Xfreed;
	Z3VariableContainer1D Xfreednext;
	Z3VariableContainer1D Xin;
	Z3VariableContainer1D Xout;
	Z3VariableContainer1D Xinseen;
	Z3VariableContainer1D Xinv_eq;
	Z3VariableContainer1D Xinv_next;
	Z3VariableContainer1D Xinv_reach;
	Z3VariableContainer1D Xsin_eq;
	Z3VariableContainer1D Xsin_next;
	Z3VariableContainer1D Xsin_reach;
	expr Xsinout;
	int Xsinoutid;
	
	std::vector<expr> XPC;
	std::vector<expr> XOS;
	
	expr transition;
	
	virtual Variable *vardown(Variable *var) = 0;
	virtual Variable *varup(Variable *var) = 0;
	
	virtual expr pcincrement(expr &f, Statement *command);
	virtual expr pcincrement(expr &f, Condition *command, expr cond);
	expr osupdate(expr &f, LinearisationEvent *event);
	virtual expr nochange_sinout(expr &f);
	expr nochange_freed(expr &f, std::vector<Variable*> &v, Variable *except=NULL);
	expr nochange_os(expr &f, int except=-1);
	virtual expr nochange_observed_in(expr &f, std::vector<Variable*> &v);
	virtual expr nochange_observed_out(expr &f, std::vector<Variable*> &v);
	expr nochange_shape(expr &f, std::vector<Variable*> &v, Variable *except=NULL);
	expr nochange_inv(expr &f, std::vector<Variable*> &v, Variable *except=NULL);
	expr nochange_sin(expr &f, std::vector<Variable*> &v, Variable *except=NULL);
	expr nochange_ages(expr &f, std::vector<Variable*> &v, Variable *except=NULL);
	virtual expr nochange_own(expr &f, std::vector<Variable*> &v, Variable *except=NULL) = 0;
	virtual expr claim_ownership(Variable *x) = 0;
	virtual expr lose_ownership(Variable *x) = 0;
	virtual expr nuke_ownership(Variable *x) = 0;
	virtual expr copy_ownership(Variable *soure, Variable *dest) = 0;
	virtual expr nibble_ownership(Variable *nibbler, Variable *pointer) = 0;
	virtual expr nochange_seen(Variable *except=NULL) = 0;
	virtual expr input_seen(Variable *z) = 0;
	virtual expr see_input(Variable *z) = 0;
	void shapeconsistency(Frame &frame);
	virtual void memconsistency(Frame &frame);
	
	virtual expr trans_functioncall();
	expr trans_malloc(Malloc *command);
	expr trans_free(Free *command);
	expr trans_pointerassignment(PointerAssignment *command);
	expr trans_nextpointerassignment(NextPointerAssignment *command);
	expr trans_pointernextassignment(PointerNextAssignment *command);
	expr trans_inputassignment(InputAssignment *command);
	virtual expr trans_outputassignment(OutputAssignment *command);
	expr trans_return(Return *command);
	expr trans_pointercomparison(PointerComparison *command);
	expr trans_cas(CAS *command);
	expr trans_cas_success(Variable *v, Variable *c, Variable *r);
	expr trans_cas_fail(Variable *v, Variable *c, Variable *r);
	
	
	public:
	View(Program *prog, Observer *obs, context &ctxt, std::vector<Variable*> nonobsvars, std::vector<Variable*> obsvars, std::vector<Variable*> vars, std::vector<Variable*> activelocalscope, std::vector<Variable*> activenotlocalscope);
	int count_variables();
	std::vector<expr> &get_old_variables();
	std::vector<expr> &get_new_variables();
	void build_transition();
	expr &get_transition();
	
	std::vector<int> get_variables_single();
	std::vector<Z3VariableContainer1D*> get_variables_1did();
	std::vector<Z3VariableContainer1D*> get_variables_1dvar();
	std::vector<Z3VariableContainer1D*> get_variables_1dobsvar();
	std::vector<Z3VariableContainer2D*> get_variables_2dvar();
	std::vector<Z3VariableContainer2D*> get_variables_2dnextvar();
};





class OneThreadView : public View{
	private:
	Frame initial;
	Frame safety;
	
	Variable *vardown(Variable *var);
	Variable *varup(Variable *var);
	
	expr nochange_own(expr &f, std::vector<Variable*> &v, Variable *except);
	expr claim_ownership(Variable *x);
	expr lose_ownership(Variable *x);
	expr nuke_ownership(Variable *x);
	expr copy_ownership(Variable *source, Variable *dest);
	expr nibble_ownership(Variable *nibbler, Variable *pointer);
	expr nochange_seen(Variable *except=NULL);
	expr input_seen(Variable *z);
	expr see_input(Variable *z);
	
	public:
	OneThreadView(Program *prog, Observer *obs, context &ctxt, std::vector<Variable*> nonobsvars, std::vector<Variable*> obsvars, std::vector<Variable*> vars, std::vector<Variable*> activelocalscope, std::vector<Variable*> activenotlocalscope);
	void build_initial();
	void build_safety();
	Frame &get_initial();
	Frame &get_safety();
};





class TwoThreadView : public View{
	private:
	std::vector<Variable*> passivelocalscope;
	std::vector<Variable*> passivenotlocalscope;
	
	Z3VariableContainer1D pc2;
	Z3VariableContainer1D own2;
	Z3VariableContainer1D in2;
	Z3VariableContainer1D out2;
	Z3VariableContainer1D inseen2;
	expr sinout2;
	int sinout2id;
	
	Z3VariableContainer1D Xpc2;
	Z3VariableContainer1D Xown2;
	Z3VariableContainer1D Xin2;
	Z3VariableContainer1D Xout2;
	Z3VariableContainer1D Xinseen2;
	expr Xsinout2;
	int Xsinout2id;
	
	std::vector<expr> XPC2;
	
	Frame intconstraints;
	
	Variable *vardown(Variable *var);
	Variable *varup(Variable *var);
	
	expr pcincrement(expr &f, Statement *command);
	expr pcincrement(expr &f, Condition *command, expr cond);
	expr nochange_sinout(expr &f);
	expr nochange_observed_in(expr &f, std::vector<Variable*> &v);
	expr nochange_observed_out(expr &f, std::vector<Variable*> &v);
	expr nochange_own(expr &f, std::vector<Variable*> &v, Variable *except=NULL);
	expr claim_ownership(Variable *x);
	expr lose_ownership(Variable *x);
	expr nuke_ownership(Variable *x);
	expr copy_ownership(Variable *source, Variable *dest);
	expr nibble_ownership(Variable *nibbler, Variable *pointer);
	expr nochange_seen(Variable *except=NULL);
	expr input_seen(Variable *z);
	expr see_input(Variable *z);
	void memconsistency(Frame &frame);
	
	expr trans_functioncall();
	expr trans_outputassignment(OutputAssignment *command);
	
	public:
	TwoThreadView(Program *prog, Observer *obs, context &ctxt, std::vector<Variable*> nonobsvars, std::vector<Variable*> obsvars, std::vector<Variable*> vars, std::vector<Variable*> activelocalscope, std::vector<Variable*> activenotlocalscope, std::vector<Variable*> passivelocalscope, std::vector<Variable*> passivenotlocalscope);
	void build_intconstraints();
	Frame &get_intconstraints();
	std::vector<int> get_variables2_single();
	std::vector<Z3VariableContainer1D*> get_variables2_1did();
	std::vector<Z3VariableContainer1D*> get_variables2_1dvar();
	std::vector<Z3VariableContainer1D*> get_variables2_1dobsvar();
	std::vector<Z3VariableContainer2D*> get_variables2_2dvar();
	std::vector<Z3VariableContainer2D*> get_variables2_2dnextvar();
};



