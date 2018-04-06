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
 * simpleCmd := id <- a(id) | id <- jsonexp | let x = jsonexp
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
protected:
  ASTNode () {}
};

class JSONExpression : public ASTNode
{
public:
  virtual std::string JSONExpression () = 0;
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
  
  virtual WhiskAction* convert ()
  {
    return ".saved.output_"+callStmt->getForkName ();
  }
};

class Command : public ASTNode
{
};

class SimpleCommand : public Command
{
public:
  virtual WhiskAction* convert () = 0;
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
  
  virtual WhiskAction* convert ()
  {
    vector<WhiskAction*> actions;
    
    for (auto cmd : cmds) {
      actions.push_back (convert (cmd));
    }
    
    return new WhiskSequence ("SEQUENCE" + get_random_str (WHISK_SEQ_NAME_LENGTH), 
                              actions);
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
  
  virtual WhiskAction* convert ()
  {
    return WhiskProjection ("Proj_" + get_random_str (WHISK_PROJ_NAME_LENGTH), 
                            convert(getReturnExpr ()));
  }
};

class CallAction : public SimpleCommand
{
private:
  JSONIdentifier* retVal;
  ActionName actionName;
  JSONIdentifier* arg;
  static int callID;
  std::string forkName;
  
public:
  CallAction(JSONIdentifier* _retVal, ActionName _actionName, JSONIdentifier* _arg) : 
    retVal(retVal), actionName (_actionName), arg(_arg) 
  {
    retVal->setCallStmt(this);
    callID++;
  }
  
  const JSONIdentifier* getReturnValue() {return retVal;}
  const ActionName& getActionName() {return actionName;}
  const JSONIdentifier* getArgument() {return arg;}
  std::string getForkName() {return forkName;}
  
  virtual WhiskAction* convert ()
  {
    forkName = "Fork_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
    
    return new WhiskFork ("Fork_" + gen_random_str(WHISK_FORK_NAME_LENGTH), 
                          getActionName ());
  }
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
  
  virtual WhiskAction* convert ()
  {
    std::string code;
    
    code = transformation->convert ();
    
    return new WhiskProjection ("Proj_"+gen_random_str(WHISK_PROJ_NAME_LENGTH), code);
  }
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
  
  virtual std::string convert ()
  {
    return std::to_string (number);
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
  
  virtual std::string convert ()
  {
    return str;
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
  
  virtual std::string convert ()
  {
    if (boolean) {
      return "True";
    }
    else {
      return "False";
    }
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
  
  virtual std::string convert ()
  {
    std::string to_ret;
    
    to_ret = "[";
    
    for (auto expr : exprs) {
      to_ret += convert (expr);
    }
    
    to_ret = "]";
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
  
  virtual std::string convert ()
  {
    std::string to_ret;
    
    to_ret = "{";
    
    for (auto kvpair : kvpairs) {
      to_ret = "\"" + kvpair->getKey () + "\":" + convert (kvpair->getValue ()); 
    }
    
    to_ret = "}";
    
    return to_ret;
  }
};

class Input : public JSONIdentifier 
{
public:
  Input () : JSONIdentifier ("input") {}
};

class Pattern : public ASTNode
{
public:
  virtual std::string convert () = 0;
};

class PatternApplication : public JSONExpression
{
private:
  JSONExpression* _expr;
  Pattern* _pat;
  
public:
  PatternApplication (JSONExpression* _expr, Pattern* _pat): expr(_expr), pat(_pat)
  {
  }
  
  Pattern* getPattern () {return pat;}
  
  JSONExpression* getExpression () {return expr;}
  
  virtual std::string convert () 
  {
    return convert (getExpression ()) + convert (getPattern ());
  }
};

class FieldGet : public Pattern
{
private:
  std::string fieldName;
  
public:
  FieldGet (std::string _fieldName) : fieldName(_fieldName) 
  {
  }
  
  virtual std::string convert ()
  {
    return "." + fieldName;
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
  
  virtual std::string convert ()
  {
    return "[" + std::to_string (index) + "]";
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
  
  virtual std::string convert ()
  {
    return "[\"" + keyName + "\"]";
  }
};
#endif /*__AST_H__*/
