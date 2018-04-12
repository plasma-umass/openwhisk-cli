#include <string>
#include <iostream>
#include <vector>

#include "whisk_action.h"
#include "utils.h"

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

class CallAction;

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
  virtual std::string convert () = 0;
};

class JSONIdentifier : public JSONExpression
{
private:
  std::string identifier;
  CallAction* callStmt;
  
public:
  JSONIdentifier (std::string id, CallAction* _callStmt) : 
    identifier(id)
  {
    callStmt = _callStmt;
  }
  
  JSONIdentifier (std::string id) : identifier(id)
  {
    callStmt = NULL;
  }
  
  void setCallStmt(CallAction* _callStmt);
  virtual std::string convert ();
};

class Command : public ASTNode
{
public:
  enum CommandType
  {
    SimpleCommandType,
    ComplexCommandType,
    ReturnCommandType
  } type;
protected:
  Command(CommandType _type) : type(_type) {}
  
public:
  CommandType getType() {return type;}
  virtual ~Command () {}
};

class SimpleCommand : public Command
{
public:
  SimpleCommand (): Command(SimpleCommandType) {}
  virtual WhiskAction* convert () = 0;
};

class ComplexCommand : public Command
{
private:
  std::vector<SimpleCommand*> cmds;
  
public:
  ComplexCommand(std::vector<SimpleCommand*> _cmds): Command (ComplexCommandType), 
                                                    cmds(_cmds)
  {
  }
  
  const std::vector<SimpleCommand*>& getSimpleCommands() {return cmds;}
  
  virtual WhiskAction* convert ()
  {
    std::vector<WhiskAction*> actions;
    
    for (auto cmd : cmds) {
      actions.push_back (cmd->convert ());
    }
    
    return new WhiskSequence ("Sequence_" + gen_random_str (WHISK_SEQ_NAME_LENGTH), 
                              actions);
  }
};

class Return : public Command
{
private:
  JSONExpression* exp;
  
public:
  Return (JSONExpression* _exp) : Command (ReturnCommandType)
  {
    exp = _exp;
  }
  
  JSONExpression* getReturnExpr() {return exp;}
  
  virtual WhiskAction* convert ()
  {
    return new WhiskProjection ("Proj_" + gen_random_str (WHISK_PROJ_NAME_LENGTH), 
                                getReturnExpr ()->convert ());
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
    SimpleCommand(), retVal(_retVal), actionName (_actionName), arg(_arg) 
  {
    retVal->setCallStmt(this);
    callID++;
    forkName = "";
  }
  
  const JSONIdentifier* getReturnValue() {return retVal;}
  const ActionName& getActionName() {return actionName;}
  const JSONIdentifier* getArgument() {return arg;}
  std::string getForkName() 
  {
    if (forkName == "") {
      forkName = "Fork_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
    }
    
    return forkName;
  }
  
  virtual WhiskAction* convert ()
  {
    return new WhiskProjForkPair (new WhiskProjection ("Proj_"+ gen_random_str(WHISK_PROJ_NAME_LENGTH),
                                                       arg->convert ()),
                                  new WhiskFork (getForkName (), getActionName ()));
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
    SimpleCommand(), out(_out), in(_in), transformation(_trans) 
  {
  }
  
  const JSONIdentifier* getOutput() {return out;}
  const JSONIdentifier* getInput() {return in;}
  const JSONExpression* getTransformation() {return transformation;}
  
  virtual WhiskAction* convert ()
  {
    std::string code;
    
    code = transformation->convert ();
    
    return new WhiskProjection ("Proj_"+gen_random_str(WHISK_PROJ_NAME_LENGTH), code);
  }
};

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
  std::vector<JSONExpression*> exprs;
  
public:
  JSONArrayExpression (std::vector<JSONExpression*> _exprs): exprs(_exprs) 
  {
  }
  
  virtual std::string convert ()
  {
    std::string to_ret;
    
    to_ret = "[";
    
    for (auto expr : exprs) {
      to_ret += expr->convert ();
    }
    
    to_ret = "]";
  }
};

class KeyValuePair : public ASTNode
{
private:
  std::string key;
  JSONExpression* value;
  
public:
  KeyValuePair (std::string _key, JSONExpression* _value) : key(_key), value(_value)
  {
  }
  
  std::string getKey () {return key;}
  JSONExpression* getValue () {return value;}
};

class JSONObjectExpression : public JSONExpression
{
private:
  std::vector<KeyValuePair*> kvpairs;
  
public:
  JSONObjectExpression (std::vector<KeyValuePair*> _kvpairs) : kvpairs(_kvpairs) 
  {
  }
  
  virtual std::string convert ()
  {
    std::string to_ret;
    
    to_ret = "{";
    
    for (auto kvpair : kvpairs) {
      to_ret = "\"" + kvpair->getKey () + "\":" + kvpair->getValue ()->convert (); 
    }
    
    to_ret = "}";
    
    return to_ret;
  }
};

class Input : public JSONIdentifier 
{
public:
  Input () : JSONIdentifier ("input") {}
  std::string convert ()
  {    
    return ".saved.input";
  }
};

class Pattern : public ASTNode
{
public:
  virtual std::string convert () = 0;
};

class PatternApplication : public JSONExpression
{
private:
  JSONExpression* expr;
  Pattern* pat;
  
public:
  PatternApplication (JSONExpression* _expr, Pattern* _pat): expr(_expr), pat(_pat)
  {
  }
  
  Pattern* getPattern () {return pat;}
  
  JSONExpression* getExpression () {return expr;}
  
  virtual std::string convert () 
  {
    return getExpression ()->convert () + getPattern ()->convert ();
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
