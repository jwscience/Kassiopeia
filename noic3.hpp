#pragma once

#include <vector>
#include "z3variablecontainer.hpp"
#include "z3++.h"

using namespace z3;


class NextConverter;
class Mediator;
class Clause;
class Frame;
class ProofObligation;
class NoIC3;


enum clausevar {CLAUSE_POSITIVE, CLAUSE_NEGATIVE, CLAUSE_DONTCARE};


class NextConverter{
	private:
	context &ctxt;
	std::vector<expr> old_variables;
	std::vector<expr> new_variables;
	ast_vector_tpl<expr> old_z3ast;
	ast_vector_tpl<expr> new_z3ast;
	
	public:
	NextConverter(context &ctxt, std::vector<expr> old, std::vector<expr> next);
	Clause convert(Clause &oldclause);
	Frame convert(Frame &oldframe);
};




class Mediator{
	private:
	context &ctxt;
	std::vector<expr> &single_variables;
	std::vector<expr> &double_variables;
	std::vector<int> table1;
	std::vector<int> table2;
	std::vector<int> rtable1;
	std::vector<int> rtable2;
	
	void make_entry(int s, int d1, int d2);
	
	public:
	Mediator(context &ctxt, std::vector<expr> &single_variables, std::vector<expr> &double_variables);
	void add(int singleid, int double1id, int double2id);
	void add_id(Z3VariableContainer1D &single, Z3VariableContainer1D &double1, Z3VariableContainer1D &double2);
	void add_1dvar(Z3VariableContainer1D &single, Z3VariableContainer1D &double1, Z3VariableContainer1D &double2, std::vector<Variable*> &vars);
	void add_2dvar(Z3VariableContainer2D &single, Z3VariableContainer2D &double1, Z3VariableContainer2D &double2, std::vector<Variable*> &vars);
	void add_2dnextvar(Z3VariableContainer2D &single, Z3VariableContainer2D &double1, Z3VariableContainer2D &double2, std::vector<Variable*> &vars);
	void check_complete();
	Clause evolve(Clause &original, int type);
	Frame evolve(Frame &original, int type);
	Frame mitosis(Frame &original);
	void cytokinesis(Clause &original, Clause &result1, Clause &result2);
	void singleadd(Frame &frame, Clause &clause, int type);
	void doubleadd(Frame &frame, Clause &clause);
};




class Clause{
	private:
	int num_vars;
	std::vector<clausevar> dirac;
	std::vector<int> contained;
	expr formula;
	expr coformula;
	
	public:
	Clause(context &ctxt, int size);
	Clause(context &ctxt, std::vector<expr> &variables, std::vector<clausevar> stati);
	Clause(context &ctxt, std::vector<expr> &variables, model &m);
	void set_variable(expr var, int var_id, clausevar status);
	clausevar get_literal(int id);
	std::vector<clausevar> &get_dirac();
	std::vector<int> &get_contained();
	expr get_formula();
	expr get_coformula();
	bool contains(int literalid);
	bool compare(Clause &other);
	Clause remove_literal(context &ctxt, std::vector<expr> &variables, int id);
	Clause intersect(context &ctxt, std::vector<expr> &variables, Clause &other);
};




class Frame{
	private:
	std::vector<Clause> clauses;
	expr formula;
	std::vector<Clause> notpropagated;
	bool propagated_change;
	
	public:
	Frame(context &ctxt);
	bool contains_clause(Clause &clause);
	void add_clause(Clause clause, bool propagate=true);
	std::vector<Clause> &get_clauses();
	expr &get_formula();
	void fill_solver(solver &s);
	bool has_propagation();
	std::vector<Clause> &get_propagation();
	void set_propagation(std::vector<Clause> newlist);
};




class ProofObligation{
	private:
	Clause obligation;
	bool fulfilled;
	bool failed;
	ProofObligation *twin;
	ProofObligation *backtrack;
	
	public:
	ProofObligation(Clause ob, ProofObligation *back);
	Clause get_clause();
	void set_twin(ProofObligation *tw);
	void set_fulfilled();
	void set_failed();
	bool is_fulfilled();
	bool is_failed();
};




class NoIC3{
	private:
	context &ctxt;
	std::vector<expr> &seq_old_variables;
	std::vector<expr> &seq_new_variables;
	std::vector<expr> &int_old_variables;
	std::vector<expr> &int_new_variables;
	NextConverter seqconverter;
	NextConverter intconverter;
	Mediator &mediator;
	Frame &initial;
	Frame double_initial;
	expr &transition;
	expr &inttransition;
	Frame &intconstraints;
	Frame &safety;
	Frame double_safety;
	Frame intsafety;
	Frame Xsafety;
	Frame Xintsafety;
	std::vector<Frame> frames;
	std::vector<Frame> doubleframes;
	int frontier_level;
	
	public:
	NoIC3(context &ctxt, std::vector<expr> &seq_old_variables, std::vector<expr> &seq_new_variables, std::vector<expr> &int_old_variables, std::vector<expr> &int_new_variables, Mediator &mediator, Frame &initial, expr &transition, expr &inttransition, Frame &intconstraints, Frame &safety);
	bool prove();
	bool extendFrontier();
	void propagateClauses();
	bool removeCTI(Clause &witness);
};





