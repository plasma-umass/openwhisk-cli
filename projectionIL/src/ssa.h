#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <assert.h>
#include "whisk_action.h"
#include "utils.h"
#include "ast.h"
#include "llspl.h"

#ifndef __SSA_H__
#define __SSA_H__

typedef std::string ActionName;
class IRNode;
class Expression;
class Identifier;
class Array;
class ArrayIndexPattern;
class Assignment;
class BackwardBranch;
class BasicBlock;
class Boolean;
class Call;
class Conditional;
class ConditionalBranch;
class Constant;
class DirectBranch;
class Expression;
class FieldGetPattern;
class Input;
class Instruction;
class JSONKeyValuePair;
class JSONObject;
class KeyGetPattern;
class Let;
class LoadPointer;
class Number;
class PHI;
class Pattern;
class PatternApplication;
class Patterns;
class Pointer;
class Program;
class Return;
class StorePointer;
class String;
class Transformation;

#include "ssaVisitor.h"

class IRNode
{
protected:
  IRNode () {}
public:
  virtual void print (std::ostream& os) = 0;
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg) = 0;
  virtual ~IRNode () {}
};

class Expression : public IRNode
{
public:
  virtual std::string convert () = 0;
  virtual std::string convertToLLSPL () = 0;
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg) = 0;
};

class Identifier : public Expression
{
private:
  std::string identifier;
  Call* callStmt;
  static std::unordered_map <std::string, std::vector <Identifier*> > identifiers;
  int version;
  
public:
  Identifier (std::string id, int _version, Call* _callStmt) : 
    identifier(id), version (_version), callStmt (_callStmt)
  {
    if (identifiers.find(id) == identifiers.end ())
      identifiers [id] = std::vector <Identifier*> ();
    identifiers [id].push_back (this);
  }
  
  Identifier (std::string id, int _version) : identifier(id), 
                                  version (_version), callStmt(nullptr)
  {
    if (identifiers.find(id) == identifiers.end ())
      identifiers [id] = std::vector <Identifier*> ();
    identifiers [id].push_back (this);
  }
  Identifier (std::string id) : Identifier(id, -1) {}
  
  void setVersion (int _version) {version = _version;}
  int getVersion () {return version;}
  void setCallStmt(Call* _callStmt);
  virtual std::string convert ();
  virtual std::string convertToLLSPL () {return convert ();}
  std::string getID () const {return identifier;}
  std::string getIDWithVersion () const {return identifier+"_"+std::to_string (version);}
  virtual void print (std::ostream& os)
  {
    os << identifier+"_"+std::to_string(version);
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class Instruction : public IRNode
{
protected:
  Instruction () {}
  
public:
  virtual ~Instruction () {}
  virtual std::string getActionName () = 0;
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection) = 0;
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection) = 0;
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg) = 0;
  
};

/*class UseDef 
{
private:
  Identifier* variable;
  std::vector<Instruction*> useNodes;
  Instruction* defNode;
  
public:
  UseDef (Identifier* var, std::vector<Instruction*> uses, Instruction* def) :
    variable(var), useNodes(use), defNode(def)
  {
  }
  
  Identifier* getVariable () const {return variable;}
  Instruction* getUseNodes () const {return useNodes;}
  Instruction* getDefNode () const {return defNode;}
};*/

class BasicBlock : public Instruction
{
protected:
  std::vector<Instruction*> cmds;
  std::string actionName;
  bool converted;
  ServerlessSequence* seq;
  std::string basicBlockName;
  std::vector <BasicBlock*> predecessors;
  std::vector <BasicBlock*> successors;
  std::unordered_map <Identifier*, LoadPointer*> reads;
  std::unordered_map <Identifier*, StorePointer*> writes;
  //std::unordered_map <Identifier*, UseDef*> useDef;
  
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
  
  bool hasWrite (Identifier* v)
  {
    for (auto iter : writes) {
      if (iter.first->getID () == v->getID ()) {
        if (iter.second == nullptr) 
          return false;
        return true;
      }
        
    }
    
    return false;
  }
  
  void insertRead (Identifier* v, LoadPointer* ptr) {reads[v] = ptr;}
  void insertWrite (Identifier* v, StorePointer* ptr) {writes[v] = ptr;}
  void insertRead (Identifier* v) {reads[v] = nullptr;}
  void insertWrite (Identifier* v) {writes[v] = nullptr;}
  std::unordered_map <Identifier*, LoadPointer*> getReads () {return reads;}
  std::unordered_map <Identifier*, StorePointer*> getWrites () {return writes;}
  
  void appendInstruction (Instruction* c)
  {
    if (c == nullptr) abort();
    cmds.push_back(c);
  }
  
  void prependInstruction (Instruction* c)
  {
    if (c == nullptr) abort();
    cmds.insert(cmds.begin(), c);
  }
  
  void appendPredecessor (BasicBlock* bb)
  {
    predecessors.push_back (bb);
  }
  
  void appendSuccessor (BasicBlock* bb)
  {
    successors.push_back (bb);
  }
  
  std::vector <BasicBlock*>& getPredecessors () {return predecessors;}
  std::vector <BasicBlock*>& getSuccessors () {return successors;}
  const std::vector<Instruction*>& getInstructions() {return cmds;}
  
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    std::vector<LLSPLAction*> actions;
    if (converted) {
      return (LLSPLSequence*)seq;
    }
     
    converted = true;
    
    seq = new LLSPLSequence (getActionName ());
    basicBlockCollection.push_back ((LLSPLSequence*)seq);
    
    for (auto cmd : cmds) {
      LLSPLAction* act;
      act = cmd->convertToLLSPL (basicBlockCollection);
      if (act != nullptr) {
        seq->appendAction (act);
      }
    }
    
    
    return seq;
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    std::vector<WhiskAction*> actions;
    if (converted) {
      return (WhiskSequence*)seq;
    }
     
    converted = true;
    
    seq = new WhiskSequence (getActionName ());
    basicBlockCollection.push_back ((WhiskSequence*)seq);
    
    for (auto cmd : cmds) {
      WhiskAction* act;
      act = cmd->convert (program, basicBlockCollection);
      seq->appendAction (act);
    }
    
    
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
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class Program : public IRNode
{
public:
  typedef std::unordered_map <std::string, std::string> JSONKeyAnalysis;
  typedef std::unordered_map <std::string, std::unordered_map <BasicBlock*, Instruction*>> LivenessAnalysis;
  
private:
  std::vector <BasicBlock*> basicBlocks;
  JSONKeyAnalysis jsonKeyAnalysis;
  LivenessAnalysis livenessAnalysis;
  
public:
  Program (std::vector <BasicBlock*> _basicBlocks) : basicBlocks(_basicBlocks)
  {}
  
  Program ()
  {}
  
  LivenessAnalysis& getLivenessAnalysis () {return livenessAnalysis;}
  JSONKeyAnalysis& getJSONKeyAnalysis () {return jsonKeyAnalysis;}
  
  void setJSONKeyAnalysis (JSONKeyAnalysis _jsonKeyAnalysis)
  {
    jsonKeyAnalysis = _jsonKeyAnalysis;
  }
  
  void setLivenessAnalysis (LivenessAnalysis _liveness)
  {
    livenessAnalysis = _liveness;
  }
  
  
  void addBasicBlock (BasicBlock* basicBlock)
  {
    basicBlocks.push_back (basicBlock);
  }
  
  std::vector <BasicBlock*>& getBasicBlocks ()
  {
    return basicBlocks;
  }
  
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    basicBlocks[0]->convertToLLSPL (basicBlockCollection);
    
    return new LLSPLProgram ("Program_"+gen_random_str(WHISK_SEQ_NAME_LENGTH), 
                             basicBlockCollection);
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    basicBlocks[0]->convert (program, basicBlockCollection);
    
    return new WhiskProgram ("Program_"+gen_random_str(WHISK_SEQ_NAME_LENGTH), 
                             basicBlockCollection);
  }
  
  virtual void print (std::ostream& os)
  {
    for (auto block : basicBlocks) {
      block->print (os);
    }
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class Call : public Instruction
{
protected:
  Identifier* retVal;
  ActionName actionName;
  Identifier* arg;
  std::string forkName;
  std::string projName;
  
public:
  Call (Identifier* _retVal, ActionName _actionName, Identifier* _arg) : 
    Instruction(), retVal(_retVal), actionName (_actionName), arg(_arg) 
  {
    retVal->setCallStmt(this);
    forkName = "Fork_" + actionName + "_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
    projName = "Proj_" + actionName + "_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
  }
  
  Identifier* getReturnValue() {return retVal;}
  virtual std::string getActionName() {return actionName;}
  Identifier* getArgument() {return arg;}
  void setReturnValue (Identifier* ret) {retVal = ret;}
  void setArgument (Identifier* _arg) {arg = _arg;}
  
  virtual std::string getForkName() 
  {
    return forkName;
  }
  
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    return new LLSPLProjForkPair (new LLSPLProjection (projName, R"(. * {\"input\": )"+arg->convert()+"}"),
                                  new LLSPLFork (getForkName (), getActionName (), 
                                  retVal->getIDWithVersion()));
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    return new WhiskProjForkPair (new WhiskProjection (projName, R"(. * {\"input\": )"+arg->convert()+"}"),
                                  new WhiskFork (getForkName (), getActionName (), 
                                                 retVal->getIDWithVersion(), 
                                                 program->getJSONKeyAnalysis ()[retVal->getIDWithVersion()]));
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
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class Pointer : public Expression
{
private:
  std::string name;

public:
  Pointer (std::string _name) : name(_name) {}
  
  std::string getName () {return name;}
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert () 
  {
    return ".saved."+name;
  }
  
  virtual void print (std::ostream& os)
  {
    os << "*"<<name;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class LoadPointer : public Instruction
{
private:
  Identifier* retVal;
  Pointer* ptr;
  std::string projName;
  
public:
  LoadPointer (Identifier* _retVal, Pointer* _ptr) : retVal(_retVal), ptr(_ptr)
  {
    projName = "Proj_Load_" + gen_random_str(WHISK_FORK_NAME_LENGTH);
  }
  
  Identifier* getRetVal () {return retVal;}
  
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    std::string retValID = retVal->getID ()+"_"+std::to_string (retVal->getVersion ());
    return new LLSPLProjection (projName,
                                R"(. * {\"saved\":{\")"+retValID+ R"(\":)"+ptr->convert () + "}}");
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
     std::string retValID = retVal->getID ()+"_"+std::to_string (retVal->getVersion ());
     return new WhiskProjection (projName,
                                 R"(. * {\"saved\":{\")"+retValID+ R"(\":)"+ptr->convert () + "}}");
  }
  
  virtual void print (std::ostream& os)
  {
    retVal->print (os);
    os << " = Load ";
    ptr->print (os);
    os << std::endl;
  }
  
  virtual std::string getForkName() 
  {
    return retVal->getID ()+"_"+std::to_string (retVal->getVersion ());
    fprintf(stderr, "LoadPointer::getForkName() should not be called\n");
    abort ();
  }
  
  virtual std::string getActionName () 
  {
    return projName;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    std::string code;
    
    code = R"(. * { \"saved\" : {\" )" + ptr->getName () + R"(\":)" + expr->convert () + "}}";
    
    return new LLSPLProjection (projName, code);
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    std::string code;
    
    code = R"(. * { \"saved\" : {\" )" + ptr->getName () + R"(\":)" + expr->convert () + "}}";
    
    return new WhiskProjection (projName, code);
  }
  
  virtual void print (std::ostream& os)
  {
    os << "Store (";
    expr->print (os);
    os << ", ";
    ptr->print (os);
    os << ");" << std::endl;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
  
  Expression* getInputExpr ()
  {
    return expr;
  }
  
  virtual std::string getActionName () {return "Store_ptr";}
};

class Return : public Instruction
{
private:
  Identifier* exp;
  std::string name;
  
public:
  Return (Identifier* _exp) : Instruction ()
  {
    exp = _exp;
    name = "Proj_" + gen_random_str (WHISK_PROJ_NAME_LENGTH);
  }
  
  Identifier* getReturnExpr() {return exp;}
  
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    return new LLSPLProjection (name, getReturnExpr ()->convert ());
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    return new WhiskProjection (name, getReturnExpr ()->convert ());
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
  
  virtual void print (std::ostream& os)
  {
    os << "return ";
    exp->print (os);
    os << std::endl;
  }
  
  virtual std::string getActionName ()
  {
    return name;
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
  
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    std::string code;
    
    code = transformation->convert ();
    
    return new LLSPLProjection (name, code);
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
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
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class Assignment : public Instruction
{
private:
  Identifier* out;
  Expression* in;
  std::string name;
  
public:
  Assignment (Identifier* _out, Expression* _in) : 
    Instruction(), out(_out), in(_in)
  {
    name = "Proj_"+gen_random_str(WHISK_PROJ_NAME_LENGTH);
  }
  
  Identifier* getOutput() const {return out;}
  Expression* getInput() const {return in;}
  
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    std::string code;
    
    code = R"(. * {\"saved\": {\")" + out->getIDWithVersion () + R"(\":)" + in->convert () + "}}";
    return new LLSPLProjection (name, code);
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    std::string code;
    
    code = R"(. * {\"saved\": {\")" + out->getIDWithVersion () + R"(\":)" + in->convert () + "}}";
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
    os << "(";
    in->print (os);
    os << ")" << std::endl;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  virtual LLSPLAction* convertToLLSPL (std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    abort ();
    return nullptr;
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    abort ();
    return nullptr;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class Conditional : public Expression
{
private:
  ConditionalOperator op;
  Expression* op1;
  Expression* op2;
  
public:
  Conditional (Expression* _op1, ConditionalOperator _op, Expression* _op2) :
    op (_op), op1(_op1), op2(_op2)
  {}
  
  Expression* getOp1 () {return op1;}
  Expression* getOp2 () {return op2;}
  ConditionalOperator getOperator () {return op;}
  
  virtual std::string convertToLLSPL () 
  {
    return op1->convert () + " " + conditionalOpConvert (op) + " " + op2->convert ();
  }
  
  virtual std::string convert () 
  {
    return op1->convert () + " " + conditionalOpConvert (op) + " " + op2->convert ();
  }
  
  virtual void print (std::ostream& os)
  {
    op1->print (os);
    os << " ";
    printConditionalOperator (os, op);
    os << " ";
    op2->print (os);
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class ConditionalBranch : public Instruction
{
private:
  Conditional* expr;
  BasicBlock* thenBranch;
  BasicBlock* elseBranch;
  std::string seqName;
  BasicBlock* parent;
  
public:
  ConditionalBranch (Conditional* _expr, BasicBlock* _thenBranch, 
                     BasicBlock* _elseBranch, BasicBlock* _parent)
  {
    expr = _expr;
    thenBranch = _thenBranch;
    elseBranch = _elseBranch;
    parent = _parent;
    thenBranch->appendPredecessor (parent);
    elseBranch->appendPredecessor (parent);
    parent->appendSuccessor (thenBranch);
    parent->appendSuccessor (elseBranch);
    seqName = "Seq_IF_THEN_ELSE_"+gen_random_str (WHISK_SEQ_NAME_LENGTH);
  }
  
  ConditionalBranch (Conditional* _expr, Instruction* _thenBranch, 
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
    parent->appendSuccessor (thenBranch);
    parent->appendSuccessor (elseBranch);
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
  
  Conditional* getCondition ()
  {
    return expr;
  }
  
  virtual LLSPLAction* convertToLLSPL(std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    std::string cond;
    LLSPLAction* thenAction;
    LLSPLAction* elseAction;
    
    cond = expr->convertToLLSPL ();
    thenAction = thenBranch->convertToLLSPL (basicBlockCollection);
    elseAction = elseBranch->convertToLLSPL (basicBlockCollection);
    
    return new LLSPLIf ("If_" + gen_random_str (WHISK_PROJ_NAME_LENGTH), cond, thenAction, elseAction);
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    WhiskAction* proj;
    WhiskSequence* thenSeq;
    WhiskSequence* elseSeq;
    WhiskSequence* toReturn;
    std::string code;
    
    code = expr->convert ();
    thenSeq = dynamic_cast <WhiskSequence*> (thenBranch->convert(program, basicBlockCollection));
    elseSeq = dynamic_cast <WhiskSequence*> (elseBranch->convert(program, basicBlockCollection));
    code = "if ("+code+R"() then (. * {\"action\": \")" + thenSeq->getName () + 
      R"(\"}) else (. * {\"action\":\")" + elseSeq->getName () + R"(\"}))";
    proj = new WhiskProjection ("Proj_"+gen_random_str (WHISK_PROJ_NAME_LENGTH),
                                code);
    toReturn = new WhiskSequence (seqName);
    toReturn->appendAction (proj);
    
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
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  Identifier* getOutput () {return output;}
  
  virtual LLSPLAction* convertToLLSPL(std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    std::string _finalString;
    int i;
    
    for (i = 0; i < commandExprVector.size () - 1; i++) {
      _finalString += "if ( . ^ "+ commandExprVector[i].second->convert().substr(1) +
        " == true) then " + commandExprVector[i].second->convert();
      _finalString += " else ( ";
    }
    
    _finalString += commandExprVector[i].second->convert();
    
    for (i = 1; i < commandExprVector.size (); i++) {
      _finalString += ")";
    }
    
    _finalString = R"(. * {\"saved\":{\")" + output->getIDWithVersion () + R"(\":)" + _finalString + "}}";
    
    return new LLSPLProjection (projName, _finalString);
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection) 
  {
    std::string _finalString;
    int i;
    
    for (i = 0; i < commandExprVector.size () - 1; i++) {
      _finalString += "if ( . ^ "+ commandExprVector[i].second->convert().substr(1) +
        " == true) then " + commandExprVector[i].second->convert();
      _finalString += " else ( ";
    }
    
    _finalString += commandExprVector[i].second->convert();
    
    for (i = 1; i < commandExprVector.size (); i++) {
      _finalString += ")";
    }
    
    _finalString = R"(. * {\"saved\":{\")" + output->getIDWithVersion () + R"(\":)" + _finalString + "}}";
    
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
      os << "(" << blockIdPair.first->getBasicBlockName() << ", ";
      blockIdPair.second->print (os);
      os << ");";
    }
    os << "]"<<std::endl;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class DirectBranch : public Instruction 
{
protected:
  BasicBlock* target;
  BasicBlock* parent;
  
public:
  DirectBranch (BasicBlock* _target, BasicBlock *_parent) : target(_target), parent(_parent)
  {
    target->appendPredecessor (parent);
    parent->appendSuccessor (target);
  }
  
  virtual LLSPLAction* convertToLLSPL(std::vector<LLSPLSequence*>& basicBlockCollection)
  {
    target->convertToLLSPL (basicBlockCollection);
    return nullptr;
    return new LLSPLDirectBranch (target->getActionName ());
  }
  
  virtual WhiskAction* convert (Program* program, std::vector<WhiskSequence*>& basicBlockCollection)
  {
    target->convert (program, basicBlockCollection);
    return new WhiskDirectBranch (target->getActionName ());
  }
  
  virtual std::string getActionName ()
  {
    fprintf (stderr, "DirectBranch::getActionName should not be called\n");
    abort ();
  }
  
  BasicBlock* getTarget () {return target;}
  
  virtual void print (std::ostream& os) 
  {
    os << "goto " << target->getBasicBlockName () << std::endl;
    //target->print (os);
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class BackwardBranch : public DirectBranch
{
public:
  BackwardBranch (BasicBlock* _target, BasicBlock *_parent) : DirectBranch (_target, _parent) {}
  
  virtual void print (std::ostream& os) 
  {
    os << "loop " << target->getBasicBlockName () << std::endl;
    //target->print (os);
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert ()
  {
    return std::to_string (number);
  }
  
  virtual void print (std::ostream& os) 
  {
    os << number;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual void print (std::ostream& os) 
  {
    os << R"(\")" << str << R"(\")";
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
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
  
  virtual void print (std::ostream& os) 
  {
    if (boolean) 
      os << "True";
    else
      os << "False";
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  std::vector<Expression*> getExpressions () const {return exprs;}
  
  virtual std::string convert ()
  {
    std::string to_ret;
    
    to_ret = "[";
    
    for (auto expr : exprs) {
      to_ret += expr->convert ();
    }
    
    to_ret = "]";
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  virtual void print (std::ostream& os) 
  {
    os << R"(\")" << key << R"(\":)";
    value->print (os);
  }
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert ()
  {
    return R"(\")" + getKey () + R"(\":)" + getValue ()->convert ();
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class JSONObject : public Expression
{
private:
  std::vector<JSONKeyValuePair*> kvpairs;
  
public:
  JSONObject (std::vector<JSONKeyValuePair*> _kvpairs) : kvpairs(_kvpairs) 
  {
  }
  
  std::vector<JSONKeyValuePair*>& getKeyValuePairs ()
  {
    return kvpairs;
  }
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert ()
  {
    std::string to_ret;
    
    to_ret = "{";
    
    if (kvpairs.size () > 0) {
      int i = 0;
      for (i = 0; i < kvpairs.size () - 1; i++) {
        to_ret += kvpairs[i]->convert () + ","; 
      }
      
      to_ret += kvpairs[i]->convert ();
    }
    
    to_ret = "}";
    
    return to_ret;
  }
  
  virtual void print (std::ostream& os) 
  {
    os << "{";
    for (auto kvpair : kvpairs) {
      kvpair->print (os);
    }
    os << "}";
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

class Input : public Identifier 
{
public:
  Input () : Identifier ("input", 0) {}
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  std::string convert ()
  {    
    return ".saved.input";
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  Expression* getIdentifier () {return expr;}
  
  virtual void print (std::ostream& os) 
  {
    expr->print (os);
    pat->print (os);
  }
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert () 
  {
    std::string pat = getPattern ()->convert ();
    std::string id = getIdentifier ()->convert ();
    
    return id + pat;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
  
  std::vector<Pattern*> getAllPatterns ()
  {
    std::vector<Pattern*> allPatterns;
    Expression* patApp = this;

    while (dynamic_cast <PatternApplication*> (patApp) != nullptr) {
      Pattern* pat = ((PatternApplication*)patApp)->getPattern ();
      
      allPatterns.insert (allPatterns.begin(), ((PatternApplication*)patApp)->getPattern ()); //Insert in reverse order
      patApp = ((PatternApplication*)patApp)->getIdentifier ();
    }
    
    return allPatterns;
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
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert ()
  {
    return "." + fieldName;
  }
  
  virtual void print (std::ostream& os) 
  {
    os << "." << fieldName;
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
  
  std::string getFieldName () {return fieldName;}
};

class ArrayIndexPattern : public Pattern
{
private:
  int index;
  
public:
  ArrayIndexPattern (int _index) : index(_index) 
  {
  }
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert ()
  {
    return "[" + std::to_string (index) + "]";
  }
  
  virtual void print (std::ostream& os) 
  {
    os << "[" << index << "]";
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
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
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert ()
  {
    return "." + keyName;
  }
  
  virtual void print (std::ostream& os) 
  {
    os << R"([\")" << keyName << R"(\"])";
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
  
  std::string getKeyName () {return keyName;}
};

class Patterns : public Pattern
{
private:
  std::vector <Pattern*> pats;

public:
  Patterns () {}
  Patterns (std::vector<Pattern*>& _pats) : pats (_pats) {}
  
  virtual std::string convertToLLSPL () 
  {
    return convert ();
  }
  
  virtual std::string convert ()
  {
    std::string toRet = "";
    for (auto pat : pats) {
      toRet += pat->convert ();
    }
    
    return toRet;
  }
  
  virtual void print (std::ostream& os) 
  {
    for (auto pat : pats) {
      pat->print (os);
    }
  }
  
  virtual void accept(IRNodeVisitor* visitor, IRNodeVisitorArg arg)
  {
    visitor->visit (this, arg);
  }
};

#endif /*__SSA_H__*/
