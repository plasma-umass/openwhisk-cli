#include <string>
#include <iostream>

#include "whisk_action.h"

#ifndef __AST_H__
#define __AST_H__
/* Previous Grammar: See Updated Grammar below
// cmd := return q | complexCmd
// complexCmd := simpleCmd+
// simpleCmd := X1 <- a(X2) | X1 <- X2(q) | let x = e | if e then cmd1 else cmd2
// q := c | [q_1, ..., q_n] | {str1: q1, ..., str2:q2} | X | input | q.id | q[n] | q[str]

// X1 <- A1 (X2)
// X2 <- X1 (q)
// return q

// X1 <- a(X2) => wsk -i action invoke a -p X2
// X1 <- X2(q) => wsk -i action create proj --proj q.js; wsk -i action invoke proj -p X2

// convert: q -> e
// q := c => c
// q := [q1, ..., qn] => [convert(q1), ..., convert(qn)]
// q := {str1:q1, ..., str2:q2} => {str1:convert(q1), ..., str2:convert(q2)}
// q := X =>
// q := input => 
// q := q.id => convert(q).id
// q := q[n] => convert(q)[n]
// q := q[str] => convert(q)[str]
*/

/* Updated Grammar
 * cmd := return jsonexp | complexCmd
 * complexCommand := simpleCmd | simpleCmd; complexCmd
 * simpleCmd := id <- a(jsonexp) | id <- jsonexp | let x = jsonexp
 * jsonexp := id | input | [jsonexp_1, ..., jsonexp_n] | {str1: jsonexp_1, ..., strN:jsonexp_n} | jsonexp pat | c
 * pat = [n] | .field | ["str"]
 * 
 * Each incoming json to proj will have two fields: output, and save.
 * input to the sequence is stored as save.input json.
 * each action will have a unique id in the sequence. The save field of json
 * will have the output sent to that action as output_<unique_id>.
 * 
 * convert (jsonexp) -> exp
 * jsonexp := c => c
 * jsonexp := [jsonexp_1, ...] => [convert(jsonexp_1), ..., convert(jsonexp_n)]
 * jsonexp := {str1: jsonexp_1, ..., strN: jsonexp_n} => {str1: convert(jsonexp_1), ..., strN:convert(jsonexp_N)}
 * jsonexp := id => get to the last action which generated this id, 
 *                 if id is generated from last action, then the expression is .output,
 *                 otherwise the expression is .saved.output_<unique_id>
 * jsonexp := input => .saved.input
 * jsonexp := jsonexp [n] => convert(jsonexp) [n]
 * jsonexp := jsonexp .field => convert(jsonexp).field
 * jsonexp := jsonexp ["str"] => convert(jsonexp)["str"]
 * 
 * X1 <- Action (input)
 * X2 <- Action (X1)
 * X3 <- Action ({"s" : X1.s; "d" : X2.s})
 * 
 * return X3
*/

typedef std::string ActionName;

class ASTVisitor 
{
};

class ASTNode
{
public:
  virtual void convert (WhiskSequence* seq) = 0;
};

class JSONExpression : public ASTNode
{
public:
};

class JSONIdentifier : public JSONExpression
{
private:
  std::string identifier;
  CallAction* callStmt;
  
public:
  JSONIdentifier (std::string id, CallAction* _callStmt = nullptr) : 
    identifier(id), callStmt(_callStmt)
  {
  }
  
  void setCallStmt(CallAction* _callStmt) {callStmt = _callStmt;}
  
  virtual void convert (WhiskSequence* seq)
  {
    
  }
};

class Command : public ASTNode
{
};

class SimpleCommand : public Command
{
};

class ComplexCommand : public Command
{
private:
  vector<SimpleCommand*> cmds;
  
public:
  ComplexCommand(vector<SimpleCommand*> _cmds): cmds(_cmds)
  {
  }
  
  const vector<SimpleCommand*>& getSimpleCommands() {return cmds;}
  
  virtual void convert ()
  {
    
  }
};

class Return : public Command
{
private:
  JSONExpression* exp;
  
public:
  Return (JSONExpression* _exp): exp(_exp) 
  {
  }
  
  const JSONExpression* getReturnExpr() {return exp;}
  
  virtual void convert ()
  {
    
  }
};

class CallAction : public SimpleCommand
{
private:
  JSONIdentifier* retVal;
  ActionName actionName;
  JSONExpression* arg;
  static int callID;
  
public:
  CallAction(JSONIdentifier* _retVal, ActionName _actionName, JSONExpression* _arg) : 
    retVal(retVal), actionName (_actionName), arg(_arg) 
  {
    retVal->setCallStmt(this);
    callID++;
  }
  
  const JSONIdentifier* getReturnValue() {return retVal;}
  const ActionName& getActionName() {return actionName;}
  const JSONExpression* getArgument() {return arg;}
};

class JSONTransformation : public SimpleCommand
{
private:
  JSONIdentifier* out;
  JSONIdentifier* in;
  JSONExpression* transformation;
  
public:
  JSONTransformation (JSONIdentifier* _out, JSONIdentifier* _in, JSONExpression* _trans) : 
    out(_out), in(_in), transformation(_trans) 
  {
  }
  
  const JSONIdentifier* getOutput() {return out;}
  const JSONIdentifier* getInput() {return in;}
  const JSONIdentifier* getTransformation() {return transformation;}
}

//~ class LetCommand : public SimpleCommand
//~ {
//~ public:
  //~ LetCommand (JSONVariable* var, Expression* expr) {
  //~ }
//~ };

//~ class IfThenElseCommand : public SimpleCommand
//~ {
//~ public:
  //~ IfThenElseCommand (Expression* expr, Command* then, Command* else) {
  //~ }
//~ };

class ConstantExpression : public JSONExpression
{
};

class NumberExpression : public ConstantExpression
{
private:
  float number;
  
public:
  NumberExpression (float _number) :number(_number)
  {
  }
};

class StringExpression : public ConstantExpression
{
private:
  std::string str;
public:
  StringExpression (std::string _str) : str(_str) 
  {
  }
};

class BooleanExpression : public ConstantExpression
{
private:
  bool boolean;
public:
  BooleanExpression (bool _boolean) : boolean(_boolean) 
  {
  }
};

class JSONArrayExpression : public JSONExpression
{
private:
  vector<JSONExpression*> exprs;
  
public:
  JSONArrayExpression (vector<JSONExpression*> _exprs): exprs(_exprs) 
  {
  }
};

class KeyValuePair : public ASTNode
{
private:
  std::string key;
  JSONExpression value;
  
public:
  KeyValuePair (std::string _key, JSONExpression* _value) : key(_key), value(_value)
  {
  }
};

class JSONObjectExpression : public JSONExpression
{
private:
  vector<KeyValuePair*> kvpairs;
  
public:
  JSONObjectExpression (vector<KeyValuePair*> _kvpairs) : kvpairs(_kvpairs) 
  {
  }
};

class Input : public JSONIdentifier 
{
public:
  Input () : JSONIdentifier ("input") {}
};

class Pattern : public ASTNode
{
};

class PatternApplication : public JSONExpression
{
private:
  JSONExpression* _expr;
  Pattern* _pat;
public:
  PatternApplication (JSONExpression* _expr, Pattern* _pat): expr(_expr), pat(_pat)
};

class FieldGet : public Pattern
{
private:
  std::string fieldName;
  
public:
  FieldGet (std::string _fieldName) : fieldName(_fieldName) 
  {
  }
};

class ArrayIndex : public Pattern
{
private:
  int index;
  
public:
  ArrayIndex (int _index) : index(_index) 
  {
  }
};

class KeyGet : public Pattern
{
private:
  std::string keyName;
  
public:
  KeyGet (std::string _keyName) : keyName(_keyName) 
  {
  }
};
#endif /*__AST_H__*/
