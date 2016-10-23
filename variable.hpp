#pragma once

#include <string>
#include <vector>



class Variable;
class SingleVariable;
class TwinVariable;


class Variable{
	protected:
	int id;
	std::string name;
	bool global;
	bool observer;
	
	public:
	Variable(std::string name, bool global, bool observer);
	virtual ~Variable();
	std::string getname();
	int getid();
	void setid(int id);
	virtual int getnextid() = 0;
	bool isglobal();
	bool isobserver();
	virtual TwinVariable *get_twin(int number);
	virtual SingleVariable *get_parent();
};

class SingleVariable : public Variable{
	private:
	static int obscount;
	static std::vector<Variable*> variables;
	TwinVariable *twin1;
	TwinVariable *twin2;
	
	public:
	SingleVariable(std::string name, bool global, bool observer);
	~SingleVariable();
	static std::vector<Variable*> get_variables();
	static std::vector<Variable*> get_obsvariables();
	static std::vector<Variable*> get_nonobsvariables();
	static std::vector<Variable*> get_twins(int number);
	int getnextid();
	void deliver_twins();
	TwinVariable *get_twin(int number);
};

class TwinVariable : public Variable{
	private:
	static int obscount;
	static std::vector<Variable*> variables;
	SingleVariable *parent;
	
	public:
	TwinVariable(std::string name, bool global, bool observer, SingleVariable *parent);
	~TwinVariable();
	static std::vector<Variable*> get_variables();
	static std::vector<Variable*> get_obsvariables();
	static std::vector<Variable*> get_nonobsvariables();
	int getnextid();
	SingleVariable *get_parent();
};







