#include <string>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include "variable.hpp"
#include "observer.hpp"
#include "ast.hpp"





int Expression::pccount = 0;

std::vector<Expression*> Expression::expressions = std::vector<Expression*>();

int Expression::loc(){
	return pccount;
}

std::vector<Expression*> Expression::get_expressions(){
	return expressions;
}

Expression::Expression(LinearisationEvent *event):
	pc(pccount),
	event(event),
	output(false)
{
	pccount++;
	expressions.push_back(this);
}

Expression::~Expression(){
}

int Expression::getpc(){
	return pc;
}

LinearisationEvent *Expression::get_event(){
	return event;
}

bool Expression::output_done(){
	return output;
}

void Expression::printline(){
	output = true;
	std::cout << "[" << pc << "]" << " ";
	printcommand();
	if(event != NULL){
		std::cout << "   ";
		event->print();
	}
	std::cout << "\n";
}





Block::~Block(){
}









expression_type Statement::get_type(){
	return STATEMENT;
}

block_type Statement::get_blocktype(){
	return BLOCK_STATEMENT;
}

Statement::Statement(LinearisationEvent *event):
	Expression(event)
{
	next = NULL;
}

Expression *Statement::getfirst(){
	return this;
}
Expression *Statement::getnext(){
	return next;
}
void Statement::setnext(Expression *expr){
	this->next = expr;
}

void Statement::printsubprogram(){
	printline();
	if(next != NULL){
		if(next->output_done()){
			std::cout << "GOTO " << next->getpc() << "\n";
		}else{
			next->printsubprogram();
		}
	}
}





Memory::Memory(Variable *assign, LinearisationEvent *event):
	Statement(event){
	this->var = assign;
}

Variable *Memory::getvar(){
	return var;
}




expression_subtype Malloc::get_subtype(){
	return MALLOC;
}

void Malloc::printcommand(){
	std::cout << var->getname() << " = malloc()";
}




expression_subtype Free::get_subtype(){
	return FREE;
}

void Free::printcommand(){
	std::cout << "free(" << var->getname() << ")";
}




Increment::Increment(Variable *var, LinearisationEvent *event):
	Statement(event){
	this->var = var;
}

Variable *Increment::getvar(){
	return var;
}




expression_subtype AgeIncrement::get_subtype(){
	return AGEINCREMENT;
}

void AgeIncrement::printcommand(){
	std::cout << var->getname() << ".age ++";
}




expression_subtype NextIncrement::get_subtype(){
	return NEXTINCREMENT;
}

void NextIncrement::printcommand(){
	std::cout << var->getname() << ".next.age ++";
}




VariableAssignment::VariableAssignment(Variable *lhs, Variable *rhs, LinearisationEvent *event):
	Statement(event){
	this->lhs = lhs;
	this->rhs = rhs;
}

Variable *VariableAssignment::getlhs(){
	return lhs;
}

Variable *VariableAssignment::getrhs(){
	return rhs;
}




expression_subtype PointerAssignment::get_subtype(){
	return POINTERASSIGNMENT;
}

void PointerAssignment::printcommand(){
	std::cout << lhs->getname() << " = " << rhs->getname();
}




expression_subtype NextPointerAssignment::get_subtype(){
	return NEXTPOINTERASSIGNMENT;
}

void NextPointerAssignment::printcommand(){
	std::cout << lhs->getname() << ".next = " << rhs->getname();
}




expression_subtype PointerNextAssignment::get_subtype(){
	return POINTERNEXTASSIGNMENT;
}

void PointerNextAssignment::printcommand(){
	std::cout << lhs->getname() << " = " << rhs->getname() << ".next";
}




expression_subtype AgeAssignment::get_subtype(){
	return AGEASSIGNMENT;
}

void AgeAssignment::printcommand(){
	std::cout << lhs->getname() << ".age = " << rhs->getname() << ".age";
}




IOAssignment::IOAssignment(Variable *var, LinearisationEvent *event):
	Statement(event){
	this->var = var;
}

Variable *IOAssignment::getvar(){
	return var;
}




expression_subtype InputAssignment::get_subtype(){
	return INPUTASSIGNMENT;
}

void InputAssignment::printcommand(){
	std::cout << var->getname() << ".data = _in_";
}




expression_subtype OutputAssignment::get_subtype(){
	return OUTPUTASSIGNMENT;
}

void OutputAssignment::printcommand(){
	std::cout << "_out_ = " << var->getname() << ".data";
}




Expression *Return::getnext(){
	return NULL;
}

void Return::setnext(Expression *next){
}

expression_subtype Return::get_subtype(){
	return RETURN;
}

void Return::printcommand(){
	std::cout << "return";
}




expression_type Condition::get_type(){
	return CONDITION;
}

Condition::Condition(bool negated, LinearisationEvent *event):
	Expression(event)
{
	this->negated = negated;
}

Expression *Condition::getnext(bool success){
	if(success) return next_success;
	return next_fail;
}

void Condition::setnext(Expression *next_success, Expression *next_fail){
	if(negated){
		this->next_success = next_fail;
		this->next_fail = next_success;
	}else{
		this->next_success = next_success;
		this->next_fail = next_fail;
	}
}

void Condition::printsubprogram(){
	printline();
	if(next_success != NULL){
		std::cout << "YES{\n";
		if(next_success->output_done()){
			std::cout << "GOTO " << next_success->getpc() << "\n";
		}else{
			next_success->printsubprogram();
		}
		std::cout << "}\n";
	}
	if(next_fail != NULL){
		std::cout << "NO{\n";
		if(next_fail->output_done()){
			std::cout << "GOTO " << next_fail->getpc() << "\n";
		}else{
			next_fail->printsubprogram();
		}
		std::cout << "}\n";
	}
}





Comparison::Comparison(Variable *lhs, Variable *rhs, bool negated, LinearisationEvent *event):
	Condition(negated, event){
	this->lhs = lhs;
	this->rhs = rhs;
}

Variable *Comparison::getlhs(){
	return lhs;
}

Variable *Comparison::getrhs(){
	return rhs;
}




expression_subtype PointerComparison::get_subtype(){
	return POINTERCOMPARISON;
}

void PointerComparison::printcommand(){
	std::cout << lhs->getname() << " == " << rhs->getname() << " ?";
}




expression_subtype AgeComparison::get_subtype(){
	return AGECOMPARISON;
}

void AgeComparison::printcommand(){
	std::cout << lhs->getname() << ".age == " << rhs->getname() << ".age ?";
}




CAS::CAS(Variable *victim, Variable *compare, Variable *replace, bool negated, LinearisationEvent *event):
	Condition(negated, event){
	this->victim = victim;
	this->compare = compare;
	this->replace = replace;
}

Variable *CAS::getvictim(){
	return victim;
}

Variable *CAS::getcompare(){
	return compare;
}

Variable *CAS::getreplace(){
	return replace;
}

expression_subtype CAS::get_subtype(){
	return CANDS;
}

void CAS::printcommand(){
	std::cout << "CompareAndSwap(" << victim->getname() << ", " << compare->getname() << ", " << replace->getname() << ") ?";
}




ProgramStructure::ProgramStructure() : Block(){
}

block_type ProgramStructure::get_blocktype(){
	return BLOCK_STRUCTURE;
}




Sequence::Sequence(std::vector<Block*> blocks) : ProgramStructure(){
	std::vector<Block*>::iterator i = blocks.begin();
	Block* old = *i;
	first = old->getfirst();
	while(++i != blocks.end()){
		old->setnext((*i)->getfirst());
		// if old is an element of program structure we can now delete it
		if(old->get_blocktype() == BLOCK_STRUCTURE){
			delete old;
		}
		old = *i;
	}
	last = old;
}

Sequence::~Sequence(){
}

Expression *Sequence::getfirst(){
	return first;
}

void Sequence::setnext(Expression *next){
	last->setnext(next);
}




Loop::Loop(Condition *condition, Sequence *body) : ProgramStructure(){
	this->condition = condition;
	this->bodyfirst = body->getfirst();
	body->setnext(condition);
	delete body;
}

void Loop::setnext(Expression *next){
	condition->setnext(bodyfirst, next);
}




Condition *Headloop::getfirst(){
	return condition;
}




Expression *Footloop::getfirst(){
	return bodyfirst;
}




IfThenElse::IfThenElse(Condition *condition, Sequence *then, Sequence *els) : ProgramStructure(){
	this->condition = condition;
	this->then = then;
	this->els = els;
	condition->setnext(then->getfirst(), els->getfirst());
}

Condition *IfThenElse::getfirst(){
	return condition;
}

void IfThenElse::setnext(Expression *next){
	then->setnext(next);
	els->setnext(next);
	
	// We don't need them anymore. Don't call this function twice!
	delete then;
	delete els;
}




IfThen::IfThen(Condition *condition, Sequence *then) : ProgramStructure(){
	this->condition = condition;
	this->then = then;
}

Condition *IfThen::getfirst(){
	return condition;
}

void IfThen::setnext(Expression *next){
	then->setnext(next);
	condition->setnext(then->getfirst(), next);
	
	// We don't need this anymore. Don't call this function twice!
	delete then;
}



