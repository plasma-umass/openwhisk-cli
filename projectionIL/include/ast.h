#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <map>
#include <utility>

#include <assert.h>

//#include "utils.h"

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

//TODO: JSONExpression in CallAction, While Condition are JSONIdentifier.
//Make them more general so they can take any expression as input.

typedef std::string ActionName;

class JSONPatternApplication;
class CallAction;
class JSONIdentifier;
class JSONAssignment;
class JSONConditional;

enum ConditionalOperator
{
  EQ, /*==*/
  NE, /* !=*/
  GE, /*>=*/
  GT, /*>*/
  LE, /*<=*/
  LT, /*<*/
  AND, /*&&*/
  OR,  /*||*/
  NOT, /* ! */
};

void printConditionalOperator (std::ostream& os, ConditionalOperator op);
std::string conditionalOpConvert (ConditionalOperator op);

class ASTVisitor 
{
};

class ASTNode
{
protected:
  ASTNode () {}
public:
  virtual void print (std::ostream& os) = 0;
  virtual ~ASTNode () {}
};

class Action : public ASTNode
{
protected:
  std::string name;
  
public:
  Action (std::string _name) : name (_name) {}
  
  std::string getName () {return name;}
  CallAction* operator () (JSONIdentifier* out, JSONIdentifier* in);
  virtual void print (std::ostream& os) {fprintf (stderr, "Action::print should never be called\n"); abort ();}
};

class JSONExpression : public ASTNode
{
public:
  virtual JSONPatternApplication& operator [] (std::string key);
  virtual JSONPatternApplication& operator [] (const char* key);
  virtual JSONPatternApplication& operator [] (int index);
  virtual JSONPatternApplication& getField (std::string fieldName);
  
  virtual JSONConditional& operator== (std::string value);
  virtual JSONConditional& operator== (float value);
  virtual JSONConditional& operator== (int value);
  virtual JSONConditional& operator== (bool value);
  virtual JSONConditional& operator== (JSONExpression& value);
  
  virtual JSONConditional& operator!= (std::string value);
  virtual JSONConditional& operator!= (float value);
  virtual JSONConditional& operator!= (int value);
  virtual JSONConditional& operator!= (bool value);
  virtual JSONConditional& operator!= (JSONExpression& value);  
  
  virtual JSONConditional& operator>= (float value);
  virtual JSONConditional& operator> (float value);
  virtual JSONConditional& operator<= (float value);
  virtual JSONConditional& operator< (float value);
  
  //virtual JSONConditional& operator&& (JSONExpression& exp);
  //virtual JSONConditional& operator|| (JSONExpression& exp);
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
    callStmt = nullptr;
  }
  
  void setCallStmt(CallAction* _callStmt);
  std::string getIdentifier () {return identifier;}
  CallAction* getCallStmt () {return callStmt;}
  
  virtual void print (std::ostream& os)
  {
    os << identifier;
  }
  
  //TODO: JSONAssignment* operator= (const JSONExpression&* exp);
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
  Command(Command& c) : type(c.type) {}
public:
  CommandType getType() {return type;}
  virtual ~Command () {}
};

class SimpleCommand : public Command
{
public:
  SimpleCommand (): Command(SimpleCommandType) {}
  virtual ~SimpleCommand () {}
};

class ComplexCommand : public Command
{
protected:
  std::vector<SimpleCommand*> cmds;
  std::string actionName;
    
public:
  ComplexCommand(std::vector<SimpleCommand*> _cmds): Command (ComplexCommandType), 
                                                    cmds(_cmds)
  {}
  
  ComplexCommand(): Command (ComplexCommandType)
  {}
  
  void operator () (SimpleCommand* cmd)
  {
    appendSimpleCommand (cmd);
  }
  
  void appendSimpleCommand (SimpleCommand* c)
  {
    cmds.push_back(c);
  }
  
  const std::vector<SimpleCommand*>& getSimpleCommands() {return cmds;}
  
  virtual std::string getActionName ()
  {
    return actionName;
  }
  
  virtual void print (std::ostream& os)
  {
    for (auto cmd : cmds) {
      cmd->print (os);
    }
  }
};

class ReturnJSON : public SimpleCommand
{
private:
  JSONExpression* exp;
  
public:
  ReturnJSON (JSONExpression* _exp) : SimpleCommand ()
  {
    exp = _exp;
  }
  
  JSONExpression* getReturnExpr() {return exp;}
  
  virtual void print (std::ostream& os)
  {
    os << "return ";
    exp->print (os);
  }
  
  virtual std::string getActionName ()
  {
    return "";
  }
};

class CallAction : public SimpleCommand
{
protected:
  JSONIdentifier* retVal;
  ActionName actionName;
  JSONExpression* arg;
  static int callID;
  
public:
  CallAction(JSONIdentifier* _retVal, ActionName _actionName, JSONExpression* _arg) : 
    SimpleCommand(), retVal(_retVal), actionName (_actionName), arg(_arg) 
  {
    //retVal->setCallStmt(this);
    callID++;
  }
  
  JSONIdentifier* getReturnValue() {return retVal;}
  virtual std::string getActionName() {return actionName;}
  JSONExpression* getArgument() {return arg;}
  
  virtual void print (std::ostream& os)
  {
    retVal->print (os);
    os << " = " << actionName << "(";
    arg->print (os);
    os << ");" << std::endl;
  }
};

//~ class JSONTransformation : public JSONExpression
//~ {
//~ private:
  //~ JSONExpression* in;
  //~ JSONPatternApplication* transformation;
  //~ std::string name;
  
//~ public:
  //~ JSONTransformation (JSONExpression* _in, JSONPatternApplication* _trans) : 
    //~ SimpleCommand(), in(_in), transformation(_trans) 
  //~ {
    //~ name = "Proj_"+gen_random_str(WHISK_PROJ_NAME_LENGTH);
  //~ }
  
  //~ JSONIdentifier* getOutput() {return out;}
  //~ JSONIdentifier* getInput() {return in;}
  //~ JSONPatternApplication* getTransformation() {return transformation;}
  
  //~ virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection);
  
  //~ virtual std::string getActionName ()
  //~ {
    //~ return name;
  //~ }
  
  
  //~ virtual void print (std::ostream& os);
//~ };

class JSONConditional : public JSONExpression
{
private:
  ConditionalOperator op;
  JSONExpression* op1;
  JSONExpression* op2;
  
public:
  JSONConditional (JSONExpression* _op1, ConditionalOperator _op, JSONExpression* _op2) :
    op (_op), op1(_op1), op2(_op2)
  {}
  
  JSONExpression* getOp1 () {return op1;}
  JSONExpression* getOp2 () {return op2;}
  ConditionalOperator getOperator () {return op;}
  
  static void printConditionalOperator (std::ostream& os, ConditionalOperator op)
  {
    switch (op) {
      case ConditionalOperator::EQ:
        os << "==";
        break;
      case ConditionalOperator::NE:
        os << "!=";
        break;
      case ConditionalOperator::GT:
        os << ">";
        break;
      case ConditionalOperator::GE:
        os << ">=";
        break;
      case ConditionalOperator::LT:
        os << "<";
        break;
      case ConditionalOperator::LE:
        os << "<=";
        break;
      case ConditionalOperator::AND:
        os << "&&";
        break;
      case ConditionalOperator::OR:
        os << "||";
        break;
      default:
        assert (false);
    }
  }
  
  virtual void print (std::ostream& os)
  {
    op1->print (os);
    printConditionalOperator (os, op);
    op2->print (os);
  }
};

class JSONAssignment : public SimpleCommand
{
private:
  JSONIdentifier* out;
  JSONExpression* in;
  
public:
  JSONAssignment (JSONIdentifier* _out, JSONExpression* _in) : 
    out(_out), in(_in)
  {
  }
  
  JSONIdentifier* getOutput() {return out;}
  JSONExpression* getInput() {return in;}
  
  virtual void print (std::ostream& os)
  {
    out->print (os);
    os << " = ";
    in->print (os);
  }
};

class LetCommand : public SimpleCommand
{
private:
  JSONExpression* expr;
  JSONIdentifier* id;

public:
  LetCommand (JSONIdentifier* _id, JSONExpression* _expr) : id(_id), expr(_expr) 
  {}
};

class IfThenElseCommand : public SimpleCommand
{
private:
  JSONConditional* expr;
  ComplexCommand* thenBranch;
  ComplexCommand* elseBranch;
  
public:
  IfThenElseCommand (JSONConditional* _expr) 
  {
    expr = _expr;
    thenBranch = new ComplexCommand ();
    elseBranch = new ComplexCommand ();
  }
  
  IfThenElseCommand (JSONConditional* _expr, ComplexCommand* _thenBranch, 
                     ComplexCommand* _elseBranch)
  {
    expr = _expr;
    thenBranch = _thenBranch;
    elseBranch = _elseBranch;
  }
  
  IfThenElseCommand (JSONConditional* _expr, SimpleCommand* _thenBranch, 
                     SimpleCommand* _elseBranch)
  {
    expr = _expr;
    thenBranch = new ComplexCommand ();
    thenBranch->appendSimpleCommand (_thenBranch);
    elseBranch = new ComplexCommand ();
    elseBranch->appendSimpleCommand (_elseBranch);
  }
  
  ComplexCommand& getThenBranch () 
  {
    return *thenBranch;
  }
  
  ComplexCommand& getElseBranch ()
  {
    return *elseBranch;
  }
  
  void setThenBranch (ComplexCommand* _thenBranch)
  {
    thenBranch = _thenBranch;
  }
  
  void setElseBranch (ComplexCommand* _elseBranch)
  {
    elseBranch = _elseBranch;
  }
  
  JSONConditional* getCondition ()
  {
    return expr;
  }
  
  virtual void print (std::ostream& os)
  {
    /*os << "if (";
    expr->print (os);
    os << ") then goto " << thenBranch->getBasicBlockName ();
    os << " else goto " << elseBranch)->getBasicBlockName () << std::endl;
    thenBranch->print (os);
    elseBranch->print (os);*/
  }
};

class WhileLoop : public SimpleCommand
{
private:
  ComplexCommand* cmds;
  JSONConditional* cond;
  
public:
  WhileLoop (JSONConditional* _cond)
  {
    cmds = new ComplexCommand ();
  }
  
  WhileLoop (JSONConditional* _cond, ComplexCommand* _cmds) : cond(_cond), cmds(_cmds)
  {}
  
  WhileLoop (JSONConditional* _cond, SimpleCommand* _cmd) : cond(_cond)
  {
    cmds = new ComplexCommand ();
    cmds->appendSimpleCommand (_cmd);
  }
  
  JSONConditional* getCondition ()
  {
    return cond;
  }
  
  ComplexCommand& getBody ()
  {
    return *cmds;
  }
  
  virtual void print (std::ostream& os)
  {
    abort ();
  }
  
  virtual std::string getActionName () {return "WhileLoop";}
};

class ConstantExpression : public JSONExpression
{
};

class NumberExpression : public ConstantExpression
{
private:
  float number;
  
public:
  NumberExpression (float _number) : number(_number)
  {
  }
  
  float getNumber ()
  {
    return number;
  }
  
  virtual void print (std::ostream& os)
  {
    os << number;
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

  std::string getString () 
  {
    return str;
  }
  
  virtual void print (std::ostream& os)
  {
    os << "\"" << str << "\"";
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
  
  void print (std::ostream& os)
  {
    if (boolean)
      os << "True";
    else
      os << "False";
  }
  
  bool getBoolean ()
  {
    return boolean;
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
  
  std::vector<KeyValuePair*>& getKVPairs ()
  {
    return kvpairs;
  }
};

class JSONInput : public JSONIdentifier 
{
public:
  JSONInput () : JSONIdentifier ("input") {}
};

class JSONPattern : public ASTNode
{
public:
};

class JSONPatternApplication : public JSONExpression
{
private:
  JSONExpression* expr;
  JSONPattern* pat;
  
public:
  JSONPatternApplication (JSONExpression* _expr, JSONPattern* _pat): expr(_expr), pat(_pat)
  {
  }
  
  JSONPattern* getPattern () {return pat;}
  JSONExpression* getExpression () {return expr;}
  
  virtual void print(std::ostream& os)
  {
    expr->print(os);
    pat->print(os);
  }
};

class FieldGetJSONPattern : public JSONPattern
{
private:
  std::string fieldName;
  
public:
  FieldGetJSONPattern (std::string _fieldName) : fieldName(_fieldName) 
  {
  }
  
  std::string getFieldName ()
  {
    return fieldName;
  }
  
  virtual void print (std::ostream& os)
  {
    os << "." << fieldName;
  }
};

class ArrayIndexJSONPattern : public JSONPattern
{
private:
  int index;
  
public:
  ArrayIndexJSONPattern (int _index) : index(_index) 
  {
  }
  
  int getIndex ()
  {
    return index;
  }
  
  virtual void print (std::ostream& os)
  {
    os << "[" << index << "]";
  }
};

class KeyGetJSONPattern : public JSONPattern
{
private:
  std::string keyName;
  
public:
  KeyGetJSONPattern (std::string _keyName) : keyName(_keyName) 
  {
  }
  
  std::string getKeyName ()
  {
    return keyName;
  }
  
  virtual void print (std::ostream& os)
  {
    os << "[\"" << keyName << "\"]";
  }
};

void convertToWhiskCommands (ComplexCommand& cmds, std::ostream& out, bool to_optimize, bool print_ssa = false);
//TODO: Add complex pattern
#endif /*__AST_H__*/
