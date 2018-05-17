#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <map>
#include <utility>

#include <assert.h>
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

//TODO: JSONExpression in CallAction, While Condition are JSONIdentifier.
//Make them more general so they can take any expression as input.

typedef std::string ActionName;

class CallAction;

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
    callStmt = nullptr;
  }
  
  void setCallStmt(CallAction* _callStmt);
  virtual std::string convert ();
  std::string getIdentifier () {return identifier;}
  CallAction* getCallStmt () {return callStmt;}
  
  virtual void print (std::ostream& os)
  {
    os << identifier;
  }
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
  virtual std::string getActionName () = 0;
  virtual WhiskAction* convert (std::vector<WhiskSequence*>& basicBlockCollection) = 0;
};

class SimpleCommand : public Command
{
public:
  SimpleCommand (): Command(SimpleCommandType) {}
  virtual WhiskAction* convert (std::vector<WhiskSequence*>& basicBlockCollection) = 0;
};

class ComplexCommand : public Command
{
protected:
  std::vector<SimpleCommand*> cmds;
  std::string actionName;
  bool converted;
  WhiskSequence* seq;
    
public:
  ComplexCommand(std::vector<SimpleCommand*> _cmds): Command (ComplexCommandType), 
                                                    cmds(_cmds)
  {
    actionName = "Sequence_" + gen_random_str (WHISK_SEQ_NAME_LENGTH);
    converted = false;
  }
  
  ComplexCommand(): Command (ComplexCommandType)
  {
    converted = false;
    actionName = "Sequence_" + gen_random_str (WHISK_SEQ_NAME_LENGTH);
  }
  
  void appendSimpleCommand (SimpleCommand* c)
  {
    cmds.push_back(c);
  }
  
  const std::vector<SimpleCommand*>& getSimpleCommands() {return cmds;}
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    std::vector<WhiskAction*> actions;
    if (converted) {
      return seq;
    }
     
    converted = true;
    
    for (auto cmd : cmds) {
      WhiskAction* act;
      act = cmd->convert (basicBlockCollection);
      actions.push_back (act);
    }
    
    seq = new WhiskSequence (getActionName (), actions);
    basicBlockCollection.push_back (seq);
    
    return seq;
  }
  
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

class ReturnJSON : public Command
{
private:
  JSONExpression* exp;
  
public:
  ReturnJSON (JSONExpression* _exp) : Command (ReturnCommandType)
  {
    exp = _exp;
  }
  
  JSONExpression* getReturnExpr() {return exp;}
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    return new WhiskProjection ("Proj_" + gen_random_str (WHISK_PROJ_NAME_LENGTH), 
                                getReturnExpr ()->convert ());
  }
};

class CallAction : public SimpleCommand
{
protected:
  JSONIdentifier* retVal;
  ActionName actionName;
  JSONExpression* arg;
  static int callID;
  std::string forkName;
  std::string projName;
  
public:
  CallAction(JSONIdentifier* _retVal, ActionName _actionName, JSONExpression* _arg) : 
    SimpleCommand(), retVal(_retVal), actionName (_actionName), arg(_arg) 
  {
    //retVal->setCallStmt(this);
    callID++;
    forkName = "Fork_" + actionName + "_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
    projName = "Proj_" + actionName + "_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
  }
  
  JSONIdentifier* getReturnValue() {return retVal;}
  virtual std::string getActionName() {return actionName;}
  JSONExpression* getArgument() {return arg;}
  virtual std::string getForkName() 
  {
    return forkName;
  }
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    return new WhiskProjForkPair (new WhiskProjection (projName, arg->convert ()),
                                  new WhiskFork (getForkName (), getActionName ()));
  }
  
  std::string getProjName ()
  {
    return projName;
  }
  
  virtual void print (std::ostream& os)
  {
    retVal->print (os);
    os << " = " << actionName << "(";
    arg->print (os);
    os << ");" << std::endl;
  }
};

class JSONTransformation : public SimpleCommand
{
private:
  JSONIdentifier* out;
  JSONIdentifier* in;
  JSONExpression* transformation;
  std::string name;
  
public:
  JSONTransformation (JSONIdentifier* _out, JSONIdentifier* _in, JSONExpression* _trans) : 
    SimpleCommand(), out(_out), in(_in), transformation(_trans) 
  {
    name = "Proj_"+gen_random_str(WHISK_PROJ_NAME_LENGTH);
  }
  
  JSONIdentifier* getOutput() {return out;}
  JSONIdentifier* getInput() {return in;}
  JSONExpression* getTransformation() {return transformation;}
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    std::string code;
    
    code = transformation->convert ();
    
    return new WhiskProjection (name, code);
  }
  
  virtual std::string getActionName ()
  {
    return name;
  }
  
  
  virtual void print (std::ostream& os)
  {
    out->print(os);
    os << " = " ;
    transformation->print (os);
    os << "(";
    in->print (os);
    os << ")" << std::endl; 
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
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    
  }
};

class IfThenElseCommand : public SimpleCommand
{
private:
  JSONExpression* expr;
  ComplexCommand* thenBranch;
  ComplexCommand* elseBranch;
  std::string seqName;
  
public:
  IfThenElseCommand (JSONExpression* _expr, ComplexCommand* _thenBranch, 
                     ComplexCommand* _elseBranch)
  {
    expr = _expr;
    thenBranch = _thenBranch;
    elseBranch = _elseBranch;
    seqName = "Seq_IF_THEN_ELSE_"+gen_random_str (WHISK_SEQ_NAME_LENGTH);
  }
  
  IfThenElseCommand (JSONExpression* _expr, SimpleCommand* _thenBranch, 
                     SimpleCommand* _elseBranch)
  {
    expr = _expr;
    thenBranch = new ComplexCommand ();
    thenBranch->appendSimpleCommand (_thenBranch);
    elseBranch = new ComplexCommand ();
    elseBranch->appendSimpleCommand (_elseBranch);
    seqName = "Seq_IF_THEN_ELSE_"+gen_random_str (WHISK_SEQ_NAME_LENGTH);
  }
  
  ComplexCommand* getThenBranch () 
  {
    return thenBranch;
  }
  
  ComplexCommand* getElseBranch ()
  {
    return elseBranch;
  }
  
  void setThenBranch (ComplexCommand* _thenBranch)
  {
    thenBranch = _thenBranch;
  }
  
  void setElseBranch (ComplexCommand* _elseBranch)
  {
    elseBranch = _elseBranch;
  }
  
  JSONExpression* getCondition ()
  {
    return expr;
  }
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    WhiskAction* proj;
    WhiskSequence* thenSeq;
    WhiskSequence* elseSeq;
    WhiskSequence* toReturn;
    std::string code;
    
    code = expr->convert ();
    thenSeq = dynamic_cast <WhiskSequence*> (thenBranch->convert(basicBlockCollection));
    elseSeq = dynamic_cast <WhiskSequence*> (elseBranch->convert(basicBlockCollection));
    code = "if ("+code+") then . * {\"action\": " + thenSeq->getName () + 
      "} else . * {\"action\":" + elseSeq->getName () + "}";
    proj = new WhiskProjection ("Proj_"+gen_random_str (WHISK_PROJ_NAME_LENGTH),
                                code);
    toReturn = new WhiskSequence (seqName);
    toReturn->appendAction (proj);
    toReturn->appendAction (new WhiskApp ());
    
    return toReturn;
  }
  
  virtual std::string getActionName () {return seqName;}
  
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
  JSONExpression* cond;
  
public:
  WhileLoop (JSONExpression* _cond, ComplexCommand* _cmds) : cond(_cond), cmds(_cmds)
  {}
  
  WhileLoop (JSONExpression* _cond, SimpleCommand* _cmd) : cond(_cond)
  {
    cmds = new ComplexCommand ();
    cmds->appendSimpleCommand (_cmd);
  }
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    //While Loop is a sequence of four actions:
    //1. Projection action. Which sees if the condition is valid and then
    //    calls other action based on it.
    //2. Sequence of actions produced by the ComplexCommand inside the while loop.
    //3. A DirectBranch action to 1.
    //4. A dummy projection action to break out of the loop.
    
    /*WhiskSequence* seq;
    WhiskSequence* cmdSeq;
    WhiskProjection* condProj;
    WhiskProjection* dummyAction;
    
    std::string ifcode;
    std::string thenCode;
    std::string elseCode;
    
    dummyAction = new WhiskProjection ("Proj_LoopEnd_"+gen_random_str(WHISK_PROJ_NAME_LENGTH), ".");
    cmdSeq = dynamic_cast <WhiskSequence*> (cmds->convert (basicBlockCollection));
    assert (cmdSeq != nullptr);
    seq = new WhiskSequence ("WhileLoop_Seq_" + gen_random_str(WHISK_SEQ_NAME_LENGTH));
    ifcode = "if (" + cond->convert () + " == true) ";
    thenCode = ". * {\"action\": " + std::string(cmdSeq->getName ()) + "}"; //TODO: Add input
    elseCode = ". * {\"action\": " + std::string(dummyAction->getName ()) + "}";
    condProj = new WhiskProjection ("Proj_WhileLoopCond_"+gen_random_str(WHISK_PROJ_NAME_LENGTH),
                                    ifcode + " then (" + thenCode + ") else (" + elseCode + ")");
    
    seq->appendAction (condProj);
    seq->appendAction (cmdSeq);
    seq->appendAction (dummyAction);
    
    return seq;    */
  }
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

class JSONInput : public JSONIdentifier 
{
public:
  JSONInput () : JSONIdentifier ("input") {}
  std::string convert ()
  {    
    return ".saved.input";
  }
};

class JSONPattern : public ASTNode
{
public:
  virtual std::string convert () = 0;
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
  
  virtual std::string convert () 
  {
    return getExpression ()->convert () + getPattern ()->convert ();
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
  
  virtual std::string convert ()
  {
    return "." + fieldName;
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
  
  virtual std::string convert ()
  {
    return "[" + std::to_string (index) + "]";
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
  
  virtual std::string convert ()
  {
    return "[\"" + keyName + "\"]";
  }
};
#endif /*__AST_H__*/
