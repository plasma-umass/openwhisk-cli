#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <utility>

#include <assert.h>
#include "whisk_action.h"
#include "utils.h"

#ifndef __SSA_H__
#define __SSA_H__

typedef std::string ActionName;

class Call;

class IRNode
{
protected:
  IRNode () {}
public:
  virtual void print (std::ostream& os) = 0;
  virtual ~IRNode () {}
};

class Expression : public IRNode
{
public:
  virtual std::string convert () = 0;
};

class Identifier : public Expression
{
private:
  std::string identifier;
  Call* callStmt;
  static std::unordered_map <std::string, std::vector <Identifier*> > identifiers;
  
public:
  Identifier (std::string id, Call* _callStmt) : 
    identifier(id), callStmt (_callStmt)
  {
    if (identifiers.find(id) == identifiers.end ())
      identifiers [id] = std::vector <Identifier*> ();
    identifiers [id].push_back (this);
  }
  
  Identifier (std::string id) : identifier(id), callStmt(nullptr)
  {
    if (identifiers.find(id) == identifiers.end ())
      identifiers [id] = std::vector <Identifier*> ();
    identifiers [id].push_back (this);
  }
  
  void setCallStmt(Call* _callStmt);
  virtual std::string convert ();
  std::string getID () const {return identifier;}
  
  virtual void print (std::ostream& os)
  {
    os << identifier;
  }
};

class Instruction : public IRNode
{
protected:
  Instruction () {}
public:
  virtual ~Instruction () {}
  virtual std::string getActionName () = 0;
  virtual WhiskAction* convert (std::vector<WhiskSequence*>& basicBlockCollection) = 0;
};

class BasicBlock : public Instruction
{
protected:
  std::vector<Instruction*> cmds;
  std::string actionName;
  bool converted;
  WhiskSequence* seq;
  std::string basicBlockName;
  std::vector <BasicBlock*> predecessors;
  
public:
  static int numberOfBasicBlocks;

  BasicBlock(std::vector<Instruction*> _cmds): Instruction (), 
                                                    cmds(_cmds)
  {
    actionName = "Sequence_" + gen_random_str (WHISK_SEQ_NAME_LENGTH);
    converted = false;
    basicBlockName = "#" + std::to_string (numberOfBasicBlocks);
    numberOfBasicBlocks++;
  }
  
  BasicBlock(): Instruction ()
  {
    converted = false;
    actionName = "Sequence_" + gen_random_str (WHISK_SEQ_NAME_LENGTH);
    basicBlockName = "#" + std::to_string (numberOfBasicBlocks);
    numberOfBasicBlocks++;
  }
  
  void appendInstruction (Instruction* c)
  {
    cmds.push_back(c);
  }
  
  void appendPredecessor (BasicBlock* bb)
  {
    predecessors.push_back (bb);
  }
  
  std::vector <BasicBlock*>& getPredecessors () {return predecessors;}
  
  const std::vector<Instruction*>& getInstructions() {return cmds;}
  
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
  
  const std::string getBasicBlockName () const {return basicBlockName;}
  
  virtual void print (std::ostream& os)
  {
    os << getBasicBlockName () <<":" << std::endl;
    for (auto cmd : cmds) {
      cmd->print (os);
    }
  }
};

class Return : public Instruction
{
private:
  Expression* exp;
  
public:
  Return (Expression* _exp) : Instruction ()
  {
    exp = _exp;
  }
  
  Expression* getReturnExpr() {return exp;}
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    return new WhiskProjection ("Proj_" + gen_random_str (WHISK_PROJ_NAME_LENGTH), 
                                getReturnExpr ()->convert ());
  }
};

class Call : public Instruction
{
protected:
  Identifier* retVal;
  ActionName actionName;
  Expression* arg;
  std::string forkName;
  std::string projName;
  
public:
  Call (Identifier* _retVal, ActionName _actionName, Expression* _arg) : 
    Instruction(), retVal(_retVal), actionName (_actionName), arg(_arg) 
  {
    retVal->setCallStmt(this);
    forkName = "Fork_" + actionName + "_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
    projName = "Proj_" + actionName + "_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
  }
  
  const Identifier* getReturnValue() {return retVal;}
  virtual std::string getActionName() {return actionName;}
  const Expression* getArgument() {return arg;}
  void setReturnValue (Identifier* ret) {retVal = ret;}
  void setArgument (Expression* _arg) {arg = _arg;}
  
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

class Transformation : public Instruction
{
private:
  Identifier* out;
  Identifier* in;
  Expression* transformation;
  std::string name;
  
public:
  Transformation (Identifier* _out, Identifier* _in, Expression* _trans) : 
    Instruction(), out(_out), in(_in), transformation(_trans) 
  {
    name = "Proj_"+gen_random_str(WHISK_PROJ_NAME_LENGTH);
  }
  
  const Identifier* getOutput() {return out;}
  const Identifier* getInput() {return in;}
  const Expression* getTransformation() {return transformation;}
  
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

class Let : public Instruction
{
private:
  Expression* expr;
  Identifier* id;

public:
  Let (Identifier* _id, Expression* _expr) : id(_id), expr(_expr) 
  {}
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    abort ();
    return nullptr;
  }
};

class ConditionalBranch : public Instruction
{
private:
  Expression* expr;
  BasicBlock* thenBranch;
  BasicBlock* elseBranch;
  std::string seqName;
  BasicBlock* parent;
  
public:
  ConditionalBranch (Expression* _expr, BasicBlock* _thenBranch, 
                     BasicBlock* _elseBranch, BasicBlock* _parent)
  {
    expr = _expr;
    thenBranch = _thenBranch;
    elseBranch = _elseBranch;
    parent = _parent;
    thenBranch->appendPredecessor (parent);
    elseBranch->appendPredecessor (parent);
    seqName = "Seq_IF_THEN_ELSE_"+gen_random_str (WHISK_SEQ_NAME_LENGTH);
  }
  
  ConditionalBranch (Expression* _expr, Instruction* _thenBranch, 
                     Instruction* _elseBranch, BasicBlock* _parent)
  {
    expr = _expr;
    parent = _parent;
    thenBranch = new BasicBlock ();
    thenBranch->appendInstruction (_thenBranch);
    thenBranch->appendPredecessor (_parent);
    elseBranch = new BasicBlock ();
    elseBranch->appendInstruction (_elseBranch);
    elseBranch->appendPredecessor (_parent);
    
    seqName = "Seq_IF_THEN_ELSE_"+gen_random_str (WHISK_SEQ_NAME_LENGTH);
  }
  
  BasicBlock* getThenBranch () 
  {
    return thenBranch;
  }
  
  BasicBlock* getElseBranch ()
  {
    return elseBranch;
  }
  
  void setThenBranch (BasicBlock* _thenBranch)
  {
    thenBranch = _thenBranch;
  }
  
  void setElseBranch (BasicBlock* _elseBranch)
  {
    elseBranch = _elseBranch;
  }
  
  Expression* getCondition ()
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
    os << "if (";
    expr->print (os);
    os << ") then goto " << thenBranch->getBasicBlockName ();
    os << " else goto " << elseBranch->getBasicBlockName () << std::endl;
    //thenBranch->print (os);
    //elseBranch->print (os);
  }
};

class PHI : public Instruction
{
private:
  std::vector<std::pair<BasicBlock*, Identifier*> > commandExprVector;
  std::string projName;
  Identifier* output;
  
public:
  PHI (Identifier* _output, 
       std::vector<std::pair<BasicBlock*, Identifier*> > _commandExprVector) :
    commandExprVector (_commandExprVector), output (_output)
  {
    projName = "Proj_"+gen_random_str(WHISK_PROJ_NAME_LENGTH);
  }
  
  const std::vector<std::pair<BasicBlock*, Identifier*> >& getCommandExprVector ()
  {
    return commandExprVector;
  }
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection) 
  {
    std::string _finalString;
    int i;
    
    for (i = 0; i < commandExprVector.size () - 1; i++) {
      _finalString += "if ("+ commandExprVector[i].second->convert() +
        " != null) then " + commandExprVector[i].second->convert();
      _finalString += " else ( ";
    }
    
    _finalString += commandExprVector[i].second->convert();
    
    for (i = 0; i < commandExprVector.size (); i++) {
      _finalString += ")";
    }
    
    return new WhiskProjection (projName, _finalString);
  }
  
  virtual std::string getActionName ()
  {
    return projName;
  }
  
  virtual void print (std::ostream& os)
  {
    output->print (os);
    os << " = ";
    os << "phi ";
    os << "[";
    
    for (auto blockIdPair : commandExprVector) {
      os << "(" << blockIdPair.first->getBasicBlockName() << ", " << blockIdPair.second->getID () << ");";
    }
    os << "]"<<std::endl;
  }
};

class Pointer : public Expression
{
private:
  std::string name;

public:
  Pointer (std::string _name) : name(_name) {}
  
  std::string getName () {return name;}
  
  virtual std::string convert () 
  {
    return ".saved."+name;
  }
};

class LoadPointer : public Call
{
private:
  Pointer* ptr;

public:
  LoadPointer (Identifier* _retVal, Pointer* _ptr) : Call (_retVal, "Load_ptr", _ptr)
  {}
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
     return new WhiskProjection (projName,
                                 ". * {\"saved\":{\""+retVal->convert () + "\":"+ptr->convert () + "}}");
  }
  
  virtual std::string getForkName() 
  {
    fprintf(stderr, "LoadPointer::getForkName() should not be called\n");
    abort ();
  }
};

class StorePointer : public Instruction
{
private:
  Expression* expr;
  Pointer* ptr;
  std::string projName;
  
public:
  StorePointer (Expression* _expr, Pointer* _ptr) : expr(_expr), ptr(_ptr) 
  {
    projName = "Proj_StorePtr_"+gen_random_str(WHISK_PROJ_NAME_LENGTH);
  }
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    std::string code;
    
    code = ". * { \"saved\" : {\" " + ptr->getName () + "\":" + expr->convert () + "}";
    
    return new WhiskProjection (projName, code);
  }
};

class DirectBranch : public Instruction 
{
private:
  BasicBlock* target;
  BasicBlock* parent;
  
public:
  DirectBranch (BasicBlock* _target, BasicBlock *_parent) : target(_target), parent(_parent)
  {
    target->appendPredecessor (parent);
  }
  
  virtual WhiskAction* convert(std::vector<WhiskSequence*>& basicBlockCollection)
  {
    target->convert (basicBlockCollection);
    return new WhiskDirectBranch (target->getActionName ());
  }
  
  virtual std::string getActionName ()
  {
    fprintf (stderr, "DirectBranch::getActionName should not be called\n");
    abort ();
  }
  
  virtual void print (std::ostream& os) 
  {
    os << "goto " << target->getBasicBlockName () << std::endl;
    //target->print (os);
  }
};

class Constant : public Expression
{
};

class Number : public Constant
{
private:
  float number;
  
public:
  Number (float _number) : number(_number)
  {
  }
  
  virtual std::string convert ()
  {
    return std::to_string (number);
  }
};

class String : public Constant
{
private:
  std::string str;
  
public:
  String (std::string _str) : str(_str) 
  {
  }
  
  virtual std::string convert ()
  {
    return str;
  }
};

class Boolean : public Constant
{
private:
  bool boolean;
  
public:
  Boolean (bool _boolean) : boolean(_boolean) 
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

class Array : public Expression
{
private:
  std::vector<Expression*> exprs;
  
public:
  Array (std::vector<Expression*> _exprs): exprs(_exprs) 
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

class JSONKeyValuePair : public IRNode
{
private:
  std::string key;
  Expression* value;
  
public:
  JSONKeyValuePair (std::string _key, Expression* _value) : key(_key), value(_value)
  {
  }
  
  std::string getKey () {return key;}
  Expression* getValue () {return value;}
};

class JSONObject : public Expression
{
private:
  std::vector<JSONKeyValuePair*> kvpairs;
  
public:
  JSONObject (std::vector<JSONKeyValuePair*> _kvpairs) : kvpairs(_kvpairs) 
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

class Input : public Identifier 
{
public:
  Input () : Identifier ("input") {}
  std::string convert ()
  {    
    return ".saved.input";
  }
};

class Pattern : public IRNode
{
public:
  virtual std::string convert () = 0;
};

class PatternApplication : public Expression
{
private:
  Expression* expr;
  Pattern* pat;
  
public:
  PatternApplication (Expression* _expr, Pattern* _pat): expr(_expr), pat(_pat)
  {
  }
  
  Pattern* getPattern () {return pat;}
  
  Expression* getExpression () {return expr;}
  
  virtual std::string convert () 
  {
    return getExpression ()->convert () + getPattern ()->convert ();
  }
};

class FieldGetPattern : public Pattern
{
private:
  std::string fieldName;
  
public:
  FieldGetPattern (std::string _fieldName) : fieldName(_fieldName) 
  {
  }
  
  virtual std::string convert ()
  {
    return "." + fieldName;
  }
};

class ArrayIndexPattern : public Pattern
{
private:
  int index;
  
public:
  ArrayIndexPattern (int _index) : index(_index) 
  {
  }
  
  virtual std::string convert ()
  {
    return "[" + std::to_string (index) + "]";
  }
};

class KeyGetPattern : public Pattern
{
private:
  std::string keyName;
  
public:
  KeyGetPattern (std::string _keyName) : keyName(_keyName) 
  {
  }
  
  virtual std::string convert ()
  {
    return "[\"" + keyName + "\"]";
  }
};
#endif /*__SSA_H__*/
