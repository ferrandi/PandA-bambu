#ifndef ExpressionParserData_H
#define ExpressionParserData_H

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sstream>

struct theNodeList;

typedef struct theNode {
    int type;
    double value; //TODO MPFR
    char* s_value; //keep it as a string
    char* name;
    bool isOutput;
    struct theNodeList* nodeArray;
} node;

typedef struct theNodeList {
    node* n;
    struct theNodeList* next; 
} nodeList;

typedef struct theVarList {
    char* name;
    struct theVarList* next; 
} varList;


typedef struct theProgram{
	nodeList* assignList;
	varList*  outVariableList;
} program;


node* createVariableNode(char* nodeName);
node* createConstantNode(char* str_value);
node* createNodeHavingRightOperandOnly(node* rightNode, int opType);


nodeList* appendNodeToListBack(node* n, nodeList* nl);
nodeList* appendNodeToListFront(node* n, nodeList* nl);
nodeList* createNodeList( node* n);

varList* appendVarToList(char* s, varList* vl);
varList* createVarList( char* s);


void printExpression(node* n);
void makeComputationalTree(node* parent, nodeList* expressionList, nodeList* statementList);
node* findVariableNodeByName(char* varName, nodeList* statementList);
bool isNodePresentInExpression(node* theNode, node* expression);
nodeList* createOuputList(nodeList* statementList, varList* variableList);

#endif
