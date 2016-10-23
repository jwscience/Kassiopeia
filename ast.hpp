#pragma once

#include <string>
#include <iostream>
#include <vector>
#include "variable.hpp"
#include "observer.hpp"


class Expression;

class Block;

class Statement;

class Memory;
class Malloc;
class Free;

class Increment;
class AgeIncrement;
class NextIncrement;

class VariableAssignment;
class PointerAssignment;
class NextPointerAssignment;
class PointerNextAssignment;

class IOAssignment;
class InputAssignment;
class OutputAssignment;

class Return;



class Condition;


class Comparison;
class PointerComparison;
class AgeComparison;
class CAS;






class ProgramStructure;

class Sequence;
class Loop;
class Headloop;
class Footloop;

class IfThenElse;
class IfThen;




enum expression_type {STATEMENT, CONDITION};
enum expression_subtype {MALLOC, FREE, AGEINCREMENT, NEXTINCREMENT, POINTERASSIGNMENT, NEXTPOINTERASSIGNMENT, POINTERNEXTASSIGNMENT, AGEASSIGNMENT, INPUTASSIGNMENT, OUTPUTASSIGNMENT, RETURN, POINTERCOMPARISON, AGECOMPARISON, CANDS};
enum block_type {BLOCK_STATEMENT, BLOCK_STRUCTURE};







class Expression{
	private:
	static int pccount;
	static std::vector<Expression*> expressions;
	int pc;
	LinearisationEvent *event;
	bool output;
	
	public:
	static int loc();
	static std::vector<Expression*> get_expressions();
	Expression(LinearisationEvent *event=NULL);
	virtual ~Expression();
	int getpc();
	LinearisationEvent *get_event();
	virtual expression_type get_type() = 0;
	virtual expression_subtype get_subtype() = 0;
	bool output_done();
	virtual void printcommand() = 0;
	void printline();
	virtual void printsubprogram() = 0;
};


class Block{
	public:
	virtual ~Block();
	virtual block_type get_blocktype() = 0;
	virtual Expression *getfirst() = 0;
	virtual void setnext(Expression *next) = 0;
};



class Statement : public Expression, public Block{
	private:
	Expression *next;
	
	public:
	Statement(LinearisationEvent *event=NULL);
	expression_type get_type();
	block_type get_blocktype();
	Expression *getfirst();
	Expression *getnext();
	virtual void setnext(Expression *expr);
	void printsubprogram();
};



class Memory : public Statement{
	protected:
	Variable *var;
	
	public:
	Memory(Variable *assign, LinearisationEvent *event=NULL);
	Variable *getvar();
};


class Malloc : public Memory{
	using Memory::Memory;
	expression_subtype get_subtype();
	void printcommand();
};


class Free : public Memory{
	using Memory::Memory;
	expression_subtype get_subtype();
	void printcommand();
};


class Increment : public Statement{
	protected:
	Variable *var;
	
	public:
	Increment(Variable *var, LinearisationEvent *event=NULL);
	Variable *getvar();
};


class AgeIncrement : public Increment{
	using Increment::Increment;
	expression_subtype get_subtype();
	void printcommand();
};


class NextIncrement : public Increment{
	using Increment::Increment;
	expression_subtype get_subtype();
	void printcommand();
};


class VariableAssignment : public Statement{
	protected:
	Variable *lhs;
	Variable *rhs;
	
	public:
	VariableAssignment(Variable *lhs, Variable *rhs, LinearisationEvent *event=NULL);
	Variable *getlhs();
	Variable *getrhs();
};

class PointerAssignment : public VariableAssignment{
	using VariableAssignment::VariableAssignment;
	
	expression_subtype get_subtype();
	void printcommand();
};


class NextPointerAssignment : public VariableAssignment{
	using VariableAssignment::VariableAssignment;
	
	expression_subtype get_subtype();
	void printcommand();
};

class PointerNextAssignment : public VariableAssignment{
	using VariableAssignment::VariableAssignment;
	
	expression_subtype get_subtype();
	void printcommand();
};

class AgeAssignment : public VariableAssignment{
	using VariableAssignment::VariableAssignment;
	
	expression_subtype get_subtype();
	void printcommand();
};


class IOAssignment : public Statement{
	protected:
	Variable *var;
	
	public:
	IOAssignment(Variable *var, LinearisationEvent *event=NULL);
	Variable *getvar();
};


class InputAssignment : public IOAssignment{
	using IOAssignment::IOAssignment;
	
	expression_subtype get_subtype();
	void printcommand();
};

class OutputAssignment : public IOAssignment{
	using IOAssignment::IOAssignment;
	
	expression_subtype get_subtype();
	void printcommand();
};



class Return : public Statement{
	using Statement::Statement;
	
	public:
	Expression *getnext();
	void setnext(Expression *next);
	expression_subtype get_subtype();
	void printcommand();
};




class Condition : public Expression{
	private:
	bool negated;
	Expression *next_success;
	Expression *next_fail;
	
	public:
	Condition(bool negated, LinearisationEvent *event=NULL);
	expression_type get_type();
	Expression *getnext(bool success);
	void setnext(Expression *next_success, Expression *next_fail);
	void printsubprogram();
};


class Comparison : public Condition{
	protected:
	Variable *lhs;
	Variable *rhs;
	
	public:
	Comparison(Variable *lhs, Variable *rhs, bool negated, LinearisationEvent *event=NULL);
	Variable *getlhs();
	Variable *getrhs();
};

class PointerComparison : public Comparison{
	using Comparison::Comparison;
	
	expression_subtype get_subtype();
	void printcommand();
};

class AgeComparison : public Comparison{
	using Comparison::Comparison;
	
	expression_subtype get_subtype();
	void printcommand();
};

class CAS : public Condition{
	private:
	Variable *victim;
	Variable *compare;
	Variable *replace;
	
	public:
	CAS(Variable *victim, Variable *compare, Variable *replace, bool negated, LinearisationEvent *event=NULL);
	Variable *getvictim();
	Variable *getcompare();
	Variable *getreplace();
	expression_subtype get_subtype();
	void printcommand();
};




class ProgramStructure : public Block{
	public:
	ProgramStructure();
	block_type get_blocktype();
};



class Sequence : public ProgramStructure{
	private:
	Expression *first;
	Block *last;
	
	public:
	Sequence(std::vector<Block*> blocks);
	virtual ~Sequence();
	Expression *getfirst();
	void setnext(Expression *next);
};


class Loop : public ProgramStructure{
	protected:
	Condition *condition;
	Expression *bodyfirst;
	
	public:
	Loop(Condition *condition, Sequence *body);
	void setnext(Expression *next);
};

class Headloop : public Loop{
	using Loop::Loop;
	
	public:
	Condition *getfirst();
};

class Footloop : public Loop{
	using Loop::Loop;
	
	public:
	Expression *getfirst();
};


class IfThenElse : public ProgramStructure{
	private:
	Condition *condition;
	Sequence *then;
	Sequence *els;
	
	public:
	IfThenElse(Condition *condition, Sequence *then, Sequence *els);
	Condition *getfirst();
	void setnext(Expression *next);
};

class IfThen : public ProgramStructure{
	private:
	Condition *condition;
	Sequence *then;
	
	public:
	IfThen(Condition *condition, Sequence *then);
	Condition *getfirst();
	void setnext(Expression *next);
};




