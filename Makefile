CXX = g++
CXXFLAGS = -Wall
LD = g++
LDFLAGS = -lz3



all: test_concurrentmadness test_concurrentsanity test_oneval test_treiberstack

clean:
	rm -f variable.o
	rm -f ast.o
	rm -f program.o
	rm -f observer.o
	rm -f noic3.o
	rm -f z3variablecontainer.o
	rm -f view.o
	rm -f analyzer.o
	rm -f stackobserver.o queueobserver.o onevalobserver.o
	rm -f concurrentmadness.o concurrentsanity.o oneval.o treiberstack.o
	rm -f test.o test_concurrentmadness test_concurrentsanity test_oneval test_treiberstack



ast.o: ast.hpp ast.cpp variable.hpp observer.hpp
	$(CXX) $(CXXFLAGS) -c -o ast.o ast.cpp

program.o: program.hpp variable.hpp ast.hpp program.cpp
	$(CXX) $(CXXFLAGS) -c -o program.o program.cpp

observer.o: observer.hpp variable.hpp observer.cpp
	$(CXX) $(CXXFLAGS) -c -o observer.o observer.cpp

variable.o: variable.hpp variable.cpp
	$(CXX) $(CXXFLAGS) -c -o variable.o variable.cpp

stackobserver.o: stackobserver.hpp stackobserver.cpp variable.hpp observer.hpp
	$(CXX) $(CXXFLAGS) -c -o stackobserver.o stackobserver.cpp

queueobserver.o: queueobserver.hpp queueobserver.cpp variable.hpp observer.hpp
	$(CXX) $(CXXFLAGS) -c -o queueobserver.o queueobserver.cpp

onevalobserver.o: onevalobserver.hpp onevalobserver.cpp variable.hpp observer.hpp
	$(CXX) $(CXXFLAGS) -c -o onevalobserver.o onevalobserver.cpp

analyzer.o: analyzer.cpp analyzer.hpp variable.hpp ast.hpp program.hpp view.hpp observer.hpp noic3.hpp
	$(CXX) $(CXXFLAGS) -c -o analyzer.o analyzer.cpp

noic3.o: noic3.hpp noic3.cpp z3variablecontainer.hpp
	$(CXX) $(CXXFLAGS) -c -o noic3.o noic3.cpp

z3variablecontainer.o: z3variablecontainer.hpp z3variablecontainer.cpp variable.hpp
	$(CXX) $(CXXFLAGS) -c -o z3variablecontainer.o z3variablecontainer.cpp

view.o: view.hpp view.cpp variable.hpp ast.hpp program.hpp observer.hpp z3variablecontainer.hpp noic3.hpp
	$(CXX) $(CXXFLAGS) -c -o view.o view.cpp



test.o: test.cpp program.hpp observer.hpp noic3.hpp analyzer.hpp
	$(CXX) $(CXXFLAGS) -c -o test.o test.cpp

concurrentmadness.o: concurrentmadness.cpp variable.hpp ast.hpp program.hpp observer.hpp view.hpp
	$(CXX) $(CXXFLAGS) -c -o concurrentmadness.o concurrentmadness.cpp

concurrentsanity.o: concurrentsanity.cpp variable.hpp ast.hpp program.hpp observer.hpp view.hpp
	$(CXX) $(CXXFLAGS) -c -o concurrentsanity.o concurrentsanity.cpp

oneval.o: oneval.cpp variable.hpp ast.hpp program.hpp observer.hpp view.hpp
	$(CXX) $(CXXFLAGS) -c -o oneval.o oneval.cpp

treiberstack.o: treiberstack.cpp variable.hpp ast.hpp program.hpp stackobserver.hpp
	$(CXX) $(CXXFLAGS) -c -o treiberstack.o treiberstack.cpp



test_concurrentmadness: test.o concurrentmadness.o variable.o ast.o program.o view.o observer.o noic3.o analyzer.o z3variablecontainer.o
	$(LD) $(LDFLAGS) -o test_concurrentmadness test.o concurrentmadness.o variable.o ast.o program.o view.o observer.o noic3.o analyzer.o z3variablecontainer.o

test_concurrentsanity: test.o concurrentsanity.o variable.o ast.o program.o view.o observer.o noic3.o analyzer.o z3variablecontainer.o
	$(LD) $(LDFLAGS) -o test_concurrentsanity test.o concurrentsanity.o variable.o ast.o program.o view.o observer.o noic3.o analyzer.o z3variablecontainer.o

test_oneval: test.o oneval.o variable.o ast.o program.o view.o observer.o onevalobserver.o noic3.o analyzer.o z3variablecontainer.o
	$(LD) $(LDFLAGS) -o test_oneval test.o oneval.o variable.o ast.o program.o view.o observer.o onevalobserver.o noic3.o analyzer.o z3variablecontainer.o

test_treiberstack: test.o variable.o ast.o program.o view.o observer.o treiberstack.o stackobserver.o noic3.o analyzer.o z3variablecontainer.o
	$(LD) $(LDFLAGS) -o test_treiberstack test.o variable.o ast.o program.o view.o observer.o treiberstack.o stackobserver.o noic3.o analyzer.o z3variablecontainer.o


