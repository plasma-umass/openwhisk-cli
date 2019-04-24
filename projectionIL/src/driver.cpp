#include "driver.h"
#include "ssa.h"

#include <vector>
#include <string>
#include <iostream>
#include <typeinfo>
#include <set>
#include <utility>
#include <queue>
#include <algorithm>
#include <sstream>

#define MAX_SEQ_NAME_SIZE 10
#define READ_VERSION -1
#define WRITE_VERSION -2

#if (READ_VERSION == WRITE_VERSION)
  #error "READ_VERSION and WRITE_VERSION are same."
#endif

//TODO: Instead of dynamic_cast use maybe enums?
//TODO: Make this language embedded in C++. Get rid of pointers?
//TODO: Add Logical Operators
//TODO-DONE: Add ConditionExpression in both IL and SSA
//TODO-DONE: Fill in the SSA functions
//TODO: Add test cases
//TODO: Add complex pattern so as to decrease number of projection action generation

typedef std::unordered_map <std::string, int> VersionMap;
typedef std::unordered_map <BasicBlock*, std::unordered_map <std::string, int> > BasicBlockVersionMap;
typedef std::unordered_map <Identifier*, std::vector<std::pair <BasicBlock*, Identifier*>>> PHINodePair;

int getProjectionTempFile (char* file, size_t size)
{
  char temp[] = "/tmp/wsk-proj-XXXXXX";
  if (mkstemp(&temp[0]) == -1) {
    fprintf (stderr, "Cannot create temporary file '%s'", temp);
    abort ();
    return -1;
  }
  
  if (size < strlen (temp)) {
    return -1;
  }
  
  strcpy (file, temp);
  return 0;
}

std::string gen_random_str(const int len)
{
  static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  std::string str = "";
    
  for (int i = 0; i < len; ++i) {
    str += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return str;
}

static std::unordered_map <std::string, Identifier*> identifiers; // <id>_<version> to Identifier*
Identifier* createNewIdentifier (std::string id, int version)
{
  std::string to_search = id+"_"+std::to_string(version);
  if (identifiers.find (to_search) == identifiers.end ()) {
    identifiers[to_search] = new Identifier(id, version);
  }
  
  return identifiers[to_search];
}

int latestVersion (std::string id, VersionMap& versionMap)
{
  if (versionMap.find (id) == versionMap.end ()) {
    versionMap [id] = 0;
  }
  
  return versionMap[id];
}

Identifier* identifierForVersion (std::string id, VersionMap& versionMap)
{
  if (versionMap.find (id) == versionMap.end ()) {
    versionMap [id] = 0;
  }
  
  return createNewIdentifier (id, versionMap[id]);
}

int updateVersionNumber (std::string id, VersionMap& versionMap, 
                         VersionMap& bbVersionMap)
{
  if (versionMap.find (id) == versionMap.end ()) {
    versionMap [id] = 0;
  } else {
    versionMap [id] += 1;
  }
  
  bbVersionMap[id] = versionMap[id];
  
  return versionMap[id];
}

void allPredsDefiningID (BasicBlock* currBasicBlock, Identifier* id,
                         BasicBlockVersionMap& bbVersionMap, 
                         PHINodePair& phiPair, 
                         std::unordered_set<BasicBlock*>& visited)
{
  if (visited.count (currBasicBlock) != 0) {
    return;
  }
  
  auto preds = currBasicBlock->getPredecessors();
  visited.insert (currBasicBlock);
  for (auto pred : preds) {
    if (visited.count (pred) == 0 and 
        bbVersionMap[pred].find (id->getID()) != bbVersionMap[pred].end ()) {
      Identifier* newID = identifierForVersion (id->getID(), bbVersionMap[pred]);
      phiPair[id].push_back (std::make_pair (pred, newID));
    } else {
      allPredsDefiningID (pred, id, bbVersionMap, phiPair, visited);      
    }
  }
}

void updateVersionNumberInSSA (IRNode* irNode, BasicBlock* basicBlock, 
                               VersionMap& idVersions, 
                               BasicBlockVersionMap& bbVersionMap,
                               PHINodePair& phiNodePair)
{
  if (dynamic_cast <Identifier*> (irNode) != nullptr) {
    Identifier* id;
    PHINodePair phiNodePairForID;
    std::unordered_set <BasicBlock*> visited;
    id = (Identifier*) (irNode);
    allPredsDefiningID (basicBlock, id, bbVersionMap, phiNodePairForID,
                        visited);
    
    if (phiNodePairForID.size () > 0) {
      for (auto iter : phiNodePairForID) {
        if (iter.second.size () >= 2) {
          updateVersionNumber (iter.first->getID (), idVersions, 
                              bbVersionMap[basicBlock]);
          phiNodePair[iter.first] = iter.second; 
        }
      }
    }
    
    id->setVersion (latestVersion (id->getID (), idVersions));
  } else if (dynamic_cast <PatternApplication*> (irNode) != nullptr) {
    PatternApplication* patapp = (PatternApplication*) irNode;
    
    updateVersionNumberInSSA (patapp->getIdentifier (), basicBlock, idVersions,
                              bbVersionMap, phiNodePair);
  } else if (dynamic_cast <JSONObject*> (irNode) != nullptr) {
    JSONObject* obj;
    
    obj = (JSONObject*) irNode;
    for (auto kvpair : obj->getKeyValuePairs ()) {
      updateVersionNumberInSSA (kvpair, basicBlock, idVersions,
                              bbVersionMap, phiNodePair);
    }
    
  } else if (dynamic_cast <JSONKeyValuePair*> (irNode) != nullptr) {
    JSONKeyValuePair* kvpair;
    
    kvpair = (JSONKeyValuePair*) irNode;
    updateVersionNumberInSSA (kvpair->getValue (), basicBlock, idVersions,
                              bbVersionMap, phiNodePair);
  } else if (dynamic_cast <Conditional*> (irNode) != nullptr) {
    Conditional* cond;
    
    cond = (Conditional*) irNode;
    updateVersionNumberInSSA (cond->getOp1 (), basicBlock, idVersions,
                              bbVersionMap, phiNodePair);
    updateVersionNumberInSSA (cond->getOp2 (), basicBlock, idVersions,
                              bbVersionMap, phiNodePair);
  } else if (dynamic_cast <Constant*> (irNode) != nullptr) {
  } else {
    std::cout << __FUNCTION__ << " not implemented for " << typeid (*irNode).name() << std::endl;
    abort ();
  }
}

void updateVersionNumberInBB (BasicBlock* basicBlock, 
                              VersionMap& idVersions, 
                              BasicBlockVersionMap& bbVersionMap)
{
  std::queue <BasicBlock*> basicBlockQueue;
  std::unordered_set <BasicBlock*> visited;
  
  basicBlockQueue.push (basicBlock);
  while (basicBlockQueue.empty () == false) {
    std::vector<Instruction*> instsToPrepend;
    basicBlock = basicBlockQueue.front ();
    basicBlockQueue.pop ();
    
    if (visited.count (basicBlock) == 1) {
      continue;
    }
    
    visited.insert (basicBlock);
    //std::cout << "updateVersionNumber: basicBlock " << basicBlock->getBasicBlockName () << std::endl;
    for (auto instr : basicBlock->getInstructions ()) {
      PHINodePair phiNodePair;
      if (dynamic_cast <Call*> (instr) != nullptr) {
        Call* call;
        Identifier* arg;
        Identifier* retVal;
        
        call = dynamic_cast <Call*> (instr);
        arg = dynamic_cast<Identifier*> (call->getArgument ());
        retVal = call->getReturnValue ();
        updateVersionNumberInSSA (call->getArgument (), basicBlock, 
                                  idVersions, bbVersionMap, phiNodePair);
        if (phiNodePair.size () > 0) {
          for (auto iter : phiNodePair) {
            if (iter.second.size () > 1) {
              instsToPrepend.push_back (new PHI (iter.first, iter.second));
            }
          }
        }
        
        retVal->setVersion (updateVersionNumber (retVal->getID (), 
                                                 idVersions, 
                                                 bbVersionMap[basicBlock]));
      } else if (dynamic_cast <ConditionalBranch*> (instr) != nullptr) {
        ConditionalBranch* condBr;
        
        condBr = dynamic_cast <ConditionalBranch*> (instr);
        updateVersionNumberInSSA (condBr->getCondition (), basicBlock,
                                  idVersions, bbVersionMap, phiNodePair);
        if (phiNodePair.size () > 0) {
          for (auto iter : phiNodePair) {
            if (iter.second.size () > 1) {
              instsToPrepend.push_back (new PHI (iter.first, iter.second));
            }
          }
        }

        basicBlockQueue.push (condBr->getThenBranch ());
        basicBlockQueue.push (condBr->getElseBranch ());
        
        //assert (*condBr->getThenBranch ()->getSuccessors().begin () ==
        //        *condBr->getElseBranch ()->getSuccessors().begin ());
        //~ updateVersionNumberInBB (*condBr->getThenBranch ()->getSuccessors().begin (),
                                 //~ idVersions, bbVersionMap);
      } else if (dynamic_cast <DirectBranch*> (instr) != nullptr) {
        DirectBranch* branch;
        
        branch = (DirectBranch*) (instr);
        basicBlockQueue.push (branch->getTarget ());
      } else if (dynamic_cast <Assignment*> (instr) != nullptr) {
        Assignment* assign;
        
        assign = (Assignment*) instr;
        updateVersionNumberInSSA (assign->getOutput (), basicBlock, 
                                  idVersions, bbVersionMap, phiNodePair);
        updateVersionNumberInSSA (assign->getInput (), basicBlock, 
                                  idVersions, bbVersionMap, phiNodePair);
      } else if (dynamic_cast <StorePointer*> (instr) != nullptr) {
        StorePointer* str;
        
        str = (StorePointer*) instr;
        updateVersionNumberInSSA (((Identifier*)str->getInputExpr ()), basicBlock, idVersions, bbVersionMap, phiNodePair);
      } else if (dynamic_cast <LoadPointer*> (instr) != nullptr) {
        LoadPointer* ld;
        
        ld = (LoadPointer*) instr;
        ld->getRetVal ()->setVersion (updateVersionNumber (((Identifier*)ld->getRetVal ())->getID (),
                                  idVersions, bbVersionMap[basicBlock]));
      } else if (dynamic_cast <Return*> (instr) != nullptr) {
        updateVersionNumberInSSA (((Return*)instr)->getReturnExpr (), basicBlock, idVersions, bbVersionMap, phiNodePair);
      } else {
        fprintf (stderr, "Type '%s' not implemented\n", typeid (*instr).name());
        abort ();
      }
    }
    
    for (auto instr: instsToPrepend) {
      basicBlock->prependInstruction (instr);
    }
  }
}

IRNode* convertToSSAIR (ASTNode* astNode, BasicBlock* currBasicBlock,
                        VersionMap& idVersions, 
                        BasicBlockVersionMap& bbVersionMap);
                        
//Takes a JSONExpression Exp, adds a statement like temp = Exp, and
//returns temp
Identifier* convertInputToSSAIR (JSONExpression* astNode, BasicBlock* currBasicBlock,
                                 VersionMap& idVersions, 
                                 BasicBlockVersionMap& bbVersionMap)
{
  if (dynamic_cast <JSONIdentifier*> (astNode) != nullptr) {
    IRNode* to_ret = convertToSSAIR (astNode, currBasicBlock, idVersions, 
                                     bbVersionMap);
    assert (dynamic_cast<Identifier*> (to_ret) != nullptr);
    return (Identifier*)to_ret;
  }
  
  IRNode* exp = convertToSSAIR (astNode, currBasicBlock, idVersions, bbVersionMap);
  assert (dynamic_cast<Expression*> (exp) != nullptr);
  static int tempVersion = 0;
  Identifier* out = new Identifier ("temp"+std::to_string(tempVersion++), 0, nullptr);
  currBasicBlock->appendInstruction (new Assignment (out, dynamic_cast<Expression*> (exp)));
  
  return out;
}

IRNode* convertToSSAIR (ASTNode* astNode, BasicBlock* currBasicBlock,
                        VersionMap& idVersions, 
                        BasicBlockVersionMap& bbVersionMap)
{
  if (dynamic_cast <JSONIdentifier*> (astNode) != nullptr) {
    JSONIdentifier* jsonId;
    
    jsonId = dynamic_cast <JSONIdentifier*> (astNode);
    if (true or idVersions.count (jsonId->getIdentifier ()) == 0) {
      //return createNewIdentifier (identifierForVersion (jsonId->getIdentifier (), idVersions));
      auto to_ret = new Identifier (jsonId->getIdentifier ());
      currBasicBlock->insertRead (to_ret);
      return to_ret;
    } else {
      //~ PHINodePair phiPair;
      //~ std::string id = jsonId->getIdentifier ();
      //~ allPredsDefiningID (currBasicBlock, createNewIdentifier(id), bbVersionMap, phiPair);
      //~ if (phiPair.size () > 0) {
        //~ id = updateVersionNumber (id, idVersions, bbVersionMap[currBasicBlock]);
        //~ Identifier* newId = createNewIdentifier (id);
        //~ std::cout << "newInputID " << newId << std::endl;
        //~ for (auto iter : phiPair) {
          //~ std::cout << "iter.first = " << iter.first << std::endl;
          //~ currBasicBlock->appendInstruction (new PHI (newId, iter.second));
        //~ }
        
        //~ return newId;
        //~ for (auto iter : phiPair) {
          //~ std::cout << iter.first << std::endl;
          //~ for (auto vecIter : iter.second) {
            //~ std::cout << vecIter.first << " " << vecIter.second->getID () << std::endl;
          //~ }
        //~ }
      //~ } else {
        //~ return createNewIdentifier (identifierForVersion (jsonId->getIdentifier (), idVersions));
      //~ }
    }
  } else if (dynamic_cast <CallAction*> (astNode) != nullptr) {
    CallAction* callAction;
    std::string newOutputID;
    Identifier* newOutput;
    Identifier* newInput;
    std::string newInputID;
    std::string inputID;
    PHINodePair phiPair;
    JSONIdentifier* input;
    
    callAction = dynamic_cast <CallAction*> (astNode);
    input = ((JSONIdentifier*)callAction->getArgument ());

    if (dynamic_cast <JSONInput*> (input) != nullptr) {
      //There can only be one Input
      newInput = new Input ();
    } else {
      inputID = input->getIdentifier ();
    
      //~ allPredsDefiningID (currBasicBlock, createNewIdentifier(inputID), bbVersionMap, phiPair);
      if (false and phiPair.size () > 0) {
        //~ newInputID = updateVersionNumber (inputID, idVersions, bbVersionMap[currBasicBlock]);
        //~ newInput = createNewIdentifier (newInputID);
        //~ std::cout << "newInputID " << newInputID << std::endl;
        //~ for (auto iter : phiPair) {
          //~ std::cout << "iter.first = " << iter.first << std::endl;
          //~ currBasicBlock->appendInstruction (new PHI (newInput, iter.second));
        //~ }
        
        //~ for (auto iter : phiPair) {
          //~ std::cout << iter.first << std::endl;
          //~ for (auto vecIter : iter.second) {
            //~ std::cout << vecIter.first << " " << vecIter.second->getID () << std::endl;
          //~ }
        //~ }
      } else {
        //newInputID = identifierForVersion (inputID, idVersions);
        newInput = (Identifier*)convertToSSAIR (input, currBasicBlock, idVersions, 
                                                bbVersionMap);
      }
    }
    
    newOutput = new Identifier (callAction->getReturnValue()->getIdentifier ());
    currBasicBlock->insertWrite (newOutput);
    //~ updateVersionNumber (,
                                       //~ idVersions, 
                                       //~ bbVersionMap[currBasicBlock]);
    
    //std::cout << newOutputID << " " <<  callAction->getActionName () << " " << newInputID << std::endl;
    return new Call (newOutput, callAction->getActionName (),
                     newInput);
                     //TODO: (Expression*)convertToSSAIR (callAction->getArgument ()));
  } /*else if (dynamic_cast <JSONTransformation*> (astNode) != nullptr) {
    JSONTransformation* trans;
    PatternApplication* patapp;
    
    trans = dynamic_cast <JSONTransformation*> (astNode);
    IRNode* in = convertToSSAIR (trans->getInput (), currBasicBlock, idVersions, bbVersionMap);
    IRNode* pat = convertToSSAIR (trans->getTransformation (), currBasicBlock, idVersions, bbVersionMap);
    IRNode* out = convertToSSAIR (trans->getOutput (), currBasicBlock, idVersions, bbVersionMap);
    assert (dynamic_cast<Pattern*> (pat) != nullptr);
    assert (dynamic_cast<Identifier*> (in) != nullptr);
    assert (dynamic_cast<Identifier*> (out) != nullptr);
    
    patapp = new PatternApplication ((Identifier*)in, (Pattern*)pat);    
    return new Assignment ((Identifier*) out, patapp);
  } */else if (dynamic_cast <LetCommand*> (astNode) != nullptr) {
    abort ();
  } else if (dynamic_cast <NumberExpression*> (astNode) != nullptr) {
      return new Number ((dynamic_cast <NumberExpression*> (astNode))->getNumber ());
  } else if (dynamic_cast <StringExpression*> (astNode) != nullptr) {
    return new String ((dynamic_cast <StringExpression*> (astNode))->getString ());
  } else if (dynamic_cast <BooleanExpression*> (astNode) != nullptr) {
    return new Boolean ((dynamic_cast <BooleanExpression*> (astNode))->getBoolean ());
  } else if (dynamic_cast <JSONArrayExpression*> (astNode) != nullptr) {
  } else if (dynamic_cast <KeyValuePair*> (astNode) != nullptr) {
    KeyValuePair* kv = dynamic_cast <KeyValuePair*> (astNode);
    //TODO: call convertInputToSSAIR?
    IRNode* node = convertToSSAIR (kv->getValue (), currBasicBlock, idVersions, bbVersionMap);
    assert (dynamic_cast <JSONKeyValuePair*> (node) != nullptr);
    return new JSONKeyValuePair (kv->getKey (), (Expression*)node);
  } else if (dynamic_cast <JSONObjectExpression*> (astNode) != nullptr) {
    std::vector<JSONKeyValuePair*> jsonkvpairs;
    JSONObjectExpression* e = (JSONObjectExpression*) (astNode);
    for (KeyValuePair* kv : e->getKVPairs ()) {
      //TODO: call convertInputToSSAIR?
      IRNode* node = convertToSSAIR (kv, currBasicBlock, idVersions, bbVersionMap);
      assert (dynamic_cast <JSONKeyValuePair*> (node) != nullptr);
      jsonkvpairs.push_back ((JSONKeyValuePair*)node);
    }
    
    return new JSONObject (jsonkvpairs);
  } else if (dynamic_cast <JSONInput*> (astNode) != nullptr) {
    return new Input ();
  } else if (dynamic_cast <JSONPatternApplication*> (astNode) != nullptr) {
    JSONPatternApplication* patapp = (JSONPatternApplication*) astNode;
    fprintf (stderr, "patapp pat '%s'\n", typeid(*patapp->getPattern ()).name());
    
    IRNode* exp = convertToSSAIR (patapp->getExpression (), 
                                       currBasicBlock, idVersions, 
                                       bbVersionMap);
    IRNode* pat = convertToSSAIR (patapp->getPattern (), currBasicBlock, idVersions, bbVersionMap);
    assert (dynamic_cast <Pattern*> (pat) != nullptr);
    assert (dynamic_cast <Expression*> (exp) != nullptr);
    return new PatternApplication ((Expression*)exp, (Pattern*)pat);
  } else if (dynamic_cast <FieldGetJSONPattern*> (astNode) != nullptr) {
    FieldGetJSONPattern* pat = (FieldGetJSONPattern*)astNode;
    return new FieldGetPattern (pat->getFieldName ());
  } else if (dynamic_cast <ArrayIndexJSONPattern*> (astNode) != nullptr) {
    ArrayIndexJSONPattern* pat = (ArrayIndexJSONPattern*) astNode;
    return new ArrayIndexPattern (pat->getIndex ());
  } else if (dynamic_cast <KeyGetJSONPattern*> (astNode) != nullptr) {
    KeyGetJSONPattern* pat = (KeyGetJSONPattern*) astNode;
    return new KeyGetPattern (pat->getKeyName ());
  } else if (dynamic_cast <JSONAssignment*> (astNode) != nullptr) {
    JSONAssignment* assign;
    
    assign = (JSONAssignment*)astNode;
    IRNode* out = convertToSSAIR (assign->getOutput (), currBasicBlock, 
                                  idVersions, bbVersionMap);
    IRNode* exp = convertInputToSSAIR (assign->getInput (), currBasicBlock,
                                       idVersions, bbVersionMap);
    assert (dynamic_cast <Identifier*> (out) != nullptr);
    assert (dynamic_cast <Expression*> (exp) != nullptr);
    return new Assignment ((Identifier*)out, (Expression*)exp);
  } else if (dynamic_cast <JSONConditional*> (astNode) != nullptr) {
    JSONConditional* cond;
    IRNode* op1, *op2;
    
    cond = (JSONConditional*) astNode;
    op1 = convertInputToSSAIR (cond->getOp1 (), currBasicBlock, idVersions, 
                               bbVersionMap); 
    op2 = convertInputToSSAIR (cond->getOp2 (), currBasicBlock, idVersions, 
                               bbVersionMap);
    assert (dynamic_cast <Expression*> (op1) != nullptr);
    assert (dynamic_cast <Expression*> (op2) != nullptr);
    
    return new Conditional ((Expression*)op1, cond->getOperator (), 
                            (Expression*)op2);
  } else if (dynamic_cast <ReturnJSON*> (astNode) != nullptr) {
    ReturnJSON* returnStmt;
    
    returnStmt = (ReturnJSON*)astNode;
    IRNode* exp = convertInputToSSAIR (returnStmt->getReturnExpr (),
                                       currBasicBlock, idVersions,
                                       bbVersionMap);
    assert (dynamic_cast <Identifier*> (exp) != nullptr);
    return new Return ((Identifier*)exp);
  } else {
    fprintf (stderr, "Invalid AstNode type '%s'\n", typeid(*astNode).name());
    abort ();
  }
  
  abort ();
}

//~ void insertStoresInPredsForLoads (BasicBlock* basicBlock)
//~ {
  //~ for (auto pred : basicBlock->getPredecessors ()) {
    //~ for (auto 
  //~ }
//~ }

BasicBlock* convertToBasicBlock (ComplexCommand* complexCmd, 
                                 std::vector<BasicBlock*>& basicBlocks, 
                                 VersionMap& idVersions,
                                 BasicBlockVersionMap& bbVersionMap,
                                 BasicBlock** exitBlock)
{
  //This function takes one ComplexCommand containing all other
  //simple commands.
  
  //First convert ComplexCommand to a set of Basic Blocks connected
  //by branches. Also inserts Phi Branches. 
  //Algorithm to insert Phi Branches:
  //1. Every block stores what are the identifiers it has and their
  // latest version number
  //2. Get the Identifier for which to look for previous accesses.
  //3. Go through all the previous basic blocks of current basic block.
  //4. If a basic block along one path has that identifier then return its
  //   latest version number.
  //Above algorithm is naive and may lead to O(N^2) complexity. We 
  //can use dominators based minimal SSA.
  
  BasicBlock* firstBasicBlock = new BasicBlock ();
  BasicBlock* currBasicBlock = firstBasicBlock;
  basicBlocks.push_back (firstBasicBlock);
  
  for (auto cmd : complexCmd->getSimpleCommands()) {
    if (dynamic_cast <IfThenElseCommand*> (cmd) != nullptr) {
      BasicBlock* thenBasicBlock;
      BasicBlock* elseBasicBlock;
      IfThenElseCommand* ifThenElsecmd;
      ConditionalBranch *condBr;
      IRNode* cond;
      BasicBlock* target;
      PHINodePair phiPair;
      BasicBlock* exitBlockForThen;
      BasicBlock* exitBlockForElse;
      
      exitBlockForThen = exitBlockForElse = nullptr;
      
      ifThenElsecmd = dynamic_cast <IfThenElseCommand*> (cmd);
      thenBasicBlock = convertToBasicBlock (&ifThenElsecmd->getThenBranch (), 
                                            basicBlocks, idVersions, 
                                            bbVersionMap, &exitBlockForThen);
      elseBasicBlock = convertToBasicBlock (&ifThenElsecmd->getElseBranch (), 
                                            basicBlocks, idVersions, 
                                            bbVersionMap, &exitBlockForElse);
      cond = convertToSSAIR (ifThenElsecmd->getCondition (), 
                             currBasicBlock, idVersions, 
                             bbVersionMap);
      assert (dynamic_cast <Conditional*> (cond) != nullptr);
      condBr = new ConditionalBranch ((Conditional*)cond, thenBasicBlock, 
                                      elseBasicBlock, currBasicBlock);
      currBasicBlock->appendInstruction (condBr);
      target = new BasicBlock ();
      if (exitBlockForThen == nullptr)
        exitBlockForThen = thenBasicBlock;
      exitBlockForThen->appendInstruction (new DirectBranch (target, exitBlockForThen));
      if (exitBlockForElse == nullptr)
        exitBlockForElse = elseBasicBlock;
      exitBlockForElse->appendInstruction (new DirectBranch (target, exitBlockForElse));
      currBasicBlock = target;
      if (exitBlock != nullptr)
        *exitBlock = target;
      basicBlocks.push_back (target);
    } else if (dynamic_cast <WhileLoop*> (cmd) != nullptr) {
      WhileLoop* loop;
      BasicBlock* testBB;
      IRNode* cond;
      BasicBlock* loopBody;
      BasicBlock* loopExit;
      ConditionalBranch* condBr;
      BasicBlock* innerExitBlock;
      
      innerExitBlock = nullptr;
      loop = dynamic_cast <WhileLoop*> (cmd);
      testBB = new BasicBlock ();
      basicBlocks.push_back (testBB);
      loopBody = convertToBasicBlock (&loop->getBody (), basicBlocks, 
                                      idVersions, bbVersionMap, 
                                      &innerExitBlock);
      
      //TODO: Make sure that for every load there is a store from every path coming
      //to this block
      for (auto iter : loopBody->getReads ()) {
        LoadPointer* ld; 
        
        ld = new LoadPointer (iter.first, new Pointer (iter.first->getID ()));
        loopBody->insertRead (iter.first, ld);
        loopBody->prependInstruction (ld);
        //Add a store for all the loopBody reads in the currBasicBlock
        StorePointer* str;
        
        if (!currBasicBlock->hasWrite (iter.first)) {
          str = new StorePointer (new Identifier (iter.first->getID ()), 
                                  new Pointer (iter.first->getID ()));
          currBasicBlock->insertWrite (iter.first, str);
          currBasicBlock->appendInstruction (str);
        }
      }
      
      for (auto iter : loopBody->getWrites ()) {
        StorePointer* str;
        
        if (!loopBody->hasWrite (iter.first)) {
          str = new StorePointer (iter.first, new Pointer (iter.first->getID ()));
          loopBody->insertWrite (iter.first, str);
          loopBody->appendInstruction (str);
        }
      }
      
      loopExit = new BasicBlock ();
      if (exitBlock != nullptr)
        *exitBlock = loopExit;
      basicBlocks.push_back (loopExit);
      
      //testBB will have loopBody as one its predecessors
      if (innerExitBlock != nullptr)
        innerExitBlock->appendInstruction (new BackwardBranch (testBB, innerExitBlock)); 
      else
        loopBody->appendInstruction (new BackwardBranch (testBB, loopBody)); 
      
      cond = convertToSSAIR (loop->getCondition (),
                             testBB, idVersions,
                             bbVersionMap);
      assert (dynamic_cast <Conditional*> (cond) != nullptr);
      for (auto iter : testBB->getReads ()) {
        LoadPointer* ptr;
        
        ptr = new LoadPointer (iter.first, new Pointer (iter.first->getID()));
        testBB->insertRead (iter.first, ptr);
        testBB->prependInstruction (ptr);
        
        
        //All the variables used in test condition but not read/write
        // in/to in loopBody also have to be stored.

        bool found = false;        
        //TODO: Change this map of reads and writes from Identifier to 
        //std::string
        for (auto iter2 : loopBody->getReads()) {
          if (iter2.first->getID () == iter.first->getID ()) {
            found = true;
            break;
          }
        }
        
        if (not found) {
          StorePointer* str;
        
          str = new StorePointer (new Identifier(iter.first->getID ()), new Pointer (iter.first->getID ()));
          currBasicBlock->insertWrite (iter.first, str);
          currBasicBlock->appendInstruction (str);
        }
      }
      
      //testBB will have currBasicBlock as one of its predecessors
      currBasicBlock->appendInstruction (new DirectBranch (testBB, 
                                                           currBasicBlock));
      
      
      assert (testBB->getWrites ().size () == 0);
      condBr = new ConditionalBranch ((Conditional*)cond, loopBody, loopExit, testBB);
      testBB->appendInstruction (condBr);
      currBasicBlock = loopExit;
    } else if (dynamic_cast <ReturnJSON*>  (cmd) != nullptr) {
      IRNode* ret;
      
      ret = convertToSSAIR (cmd, currBasicBlock, idVersions, bbVersionMap);
      assert (dynamic_cast <Return*> (ret) != nullptr);
      currBasicBlock->appendInstruction ((Instruction*)ret);
      currBasicBlock = new BasicBlock ();
      basicBlocks.push_back (currBasicBlock);
      if (exitBlock != nullptr)
        *exitBlock = currBasicBlock;
    } else {
      PHINodePair phiPair;
      Instruction* ssaInstr;
      
      ssaInstr = (Instruction*)convertToSSAIR (cmd, currBasicBlock, 
                                               idVersions, bbVersionMap);
      currBasicBlock->appendInstruction (ssaInstr);
    }
  }
  
  if (exitBlock != nullptr && *exitBlock == nullptr)
    *exitBlock = firstBasicBlock;
  return firstBasicBlock;
}

Program* convertToSSA (ComplexCommand* cmd, bool print_ssa = false)
{
  BasicBlockVersionMap bbVersionMap;
  VersionMap idVersions;
  BasicBlock* firstBasicBlock;
  std::vector<BasicBlock*> basicBlocks;
  
  firstBasicBlock = convertToBasicBlock (cmd, basicBlocks, idVersions, 
                                         bbVersionMap, nullptr);
  updateVersionNumberInBB (firstBasicBlock, idVersions, bbVersionMap);
  auto it = std::find (basicBlocks.begin(), basicBlocks.end(), firstBasicBlock);
  basicBlocks.erase (it);
  basicBlocks.insert (basicBlocks.begin(), firstBasicBlock);
  if (print_ssa) {
    for (auto bb : basicBlocks) {
      bb->print (std::cout);
    }
  }
  
  return new Program (basicBlocks);
}

void livenessAnalysis (Program* program) 
{
  /* Find the last Instruction (s) (and the BasicBlock (s)), where an Identifier
   * is used. We will emit a projection to delete that identifier from the
   * saved state.
   * */
  //Map of Identifier to BasicBlock to Instruction, where Identifier is used for last time.
  std::unordered_map <std::string, std::unordered_map <BasicBlock*, Instruction*>>  idToLastDef;
  std::unordered_set <BasicBlock*> visited;
  BasicBlock* firstBasicBlock;
  std::queue <BasicBlock*> queue;
  BasicBlock dummyFirstBlock;
  firstBasicBlock = program->getBasicBlocks ()[0];
  dummyFirstBlock.appendSuccessor (firstBasicBlock);
  
  queue.push (&dummyFirstBlock);
  
  while (queue.empty () == false) {
    /* A level search, where for each identifier we determine the basic blocks
     * (and instructions) where that variable is used. We find the last
     * instruction in each basic block, where that variable is used.
     * */
    BasicBlock* currBlock;
    std::unordered_map <std::string, std::unordered_map <BasicBlock*, Instruction*>> strToLastDefs;
    int queueSize = queue.size ();
    int levelIter = 0;
    std::queue<BasicBlock*> levelQueue;
    while (levelIter < queueSize) {
      currBlock = queue.front ();
      queue.pop ();

      for (auto block : currBlock->getSuccessors ()) {
        if (visited.count (block) == 1) 
          continue;
        visited.insert (block);
        levelQueue.push (block);
        
        for (auto instrIter = block->getInstructions ().begin ();
             instrIter != block->getInstructions ().end (); ++instrIter) {
          
          Instruction* instr = *instrIter;
          UseDef useDef;
          UseDefVisitor visitor;
          
          useDef = visitor.getAllUseDef (instr);
          for (auto varAndUses : useDef.getUses ()) {
            std::string strID = varAndUses.first;
            if (strToLastDefs.count (strID) == 0) {
              strToLastDefs[strID] = std::unordered_map <BasicBlock*, Instruction*> ();
            } 
            
            strToLastDefs[strID][block] = instr;
          }        
        }
      }
      
      levelIter++;
    }

    while (levelQueue.empty () == false) {
      queue.push (levelQueue.front ());
      levelQueue.pop ();
    }
    
    //Update the global data structure from the local information.
    for (auto strToDef : strToLastDefs) {
      idToLastDef [strToDef.first] = strToDef.second;
    }
  }
  
  program->setLivenessAnalysis (idToLastDef);
}

void jsonLivenessAnalysis (Program* program)
{
  /* This analysis is like live analysis but works at the json key/value
   * pairs instead of variables. Find if there are only specific 
   * keys of a json returned from call is used and save only those keys in
   * the state variable.
   */
   
  UseDef useDef;
  UseDefVisitor visitor;
  std::unordered_map <std::string, std::vector<std::vector<Pattern*>>> idToPatterns;
  std::unordered_set<std::string> fullyUsedId;
  Program::JSONKeyAnalysis requiredPatterns;
  useDef = visitor.getAllUseDef (program);
  for (auto varAndDefs : useDef.getDefs ()) {
    std::string id = varAndDefs.first;
    Instruction* def = varAndDefs.second;
    
    //Consider only those defs which are from call instructions.
    if (dynamic_cast<Call*> (def) == nullptr) {
      continue;
    }
    
    std::unordered_set<Instruction*> uses;
    uses = useDef.getUses () [id];
    
    for (auto use : uses) {
      //If any of these uses, use full identifier then we cannot consider
      //this identifier.
      //All the partial uses will come in Patterns and Pattern
      //are assigned through Assignment
      if (fullyUsedId.count (id) == 1) {
        continue;
      }
      std::vector<Pattern*> patternsForUse;
      if (dynamic_cast <Assignment*> (use) != nullptr) {
        Assignment* assign;
        
        assign = (Assignment*) use;
        
        if (dynamic_cast <PatternApplication*> (((Assignment*)use)->getInput ()) != nullptr) {
          if (idToPatterns.count (id) == 0) {
            idToPatterns[id] = std::vector<std::vector<Pattern*>> ();
          }
          
          //Get all the patterns used in this use satement
          PatternApplication* patApp;
          std::vector<Pattern*> allPats;
          std::vector<Pattern*> _allPats;
                    
          patApp = (PatternApplication*)((Assignment*)use)->getInput ();
          allPats = patApp->getAllPatterns ();
          bool foundArrayIndexPat = false;
          
          for (auto iter = allPats.begin(); iter != allPats.end(); ++iter) {
            Pattern* pat = *iter;
            
            if (dynamic_cast <ArrayIndexPattern*> (pat) != nullptr) {
              //idToPatterns.erase (id);
              //fullyUsedId.insert (id);
              foundArrayIndexPat = true;
              break;
            }
            
            _allPats.push_back (pat);
          }
            
          std::vector<Pattern*> patternsForUse = _allPats;
          
          if (!foundArrayIndexPat) {
            //TODO: Write a better comment
            //Get all the patterns from the use of this variable 
            //and other defined temp variables
            std::string nextId = assign->getOutput ()->getIDWithVersion ();
            std::queue <std::string> nextIdQueue;
            nextIdQueue.push (nextId);
            
            while (nextIdQueue.empty () == false) {
              nextId = nextIdQueue.front ();
              nextIdQueue.pop ();
              //Explore next temp variables in a level order fashion
              for (auto nextIdUse : useDef.getUses ()[nextId]) {
                if (dynamic_cast<Assignment*> (nextIdUse) != nullptr) {
                  if (dynamic_cast <PatternApplication*> (((Assignment*)nextIdUse)->getInput ()) != nullptr) {
                    Assignment* a;
                    a = (Assignment*)nextIdUse;
  
                    PatternApplication* patApp = (PatternApplication*)((Assignment*)use)->getInput ();
                    bool foundArrayIndexPat = false;
                    
                    std::vector<Pattern*> allPats = patApp->getAllPatterns ();
                    std::vector<Pattern*> _allPats;
                    
                    for (auto pat : allPats) {
                      if (dynamic_cast <ArrayIndexPattern*> (pat) != nullptr) {
                        foundArrayIndexPat = true;
                        break;
                      }
                      
                      _allPats.push_back (pat);
                    }
  
                    if (foundArrayIndexPat) {
                      //idToPatterns.erase (id);
                      //fullyUsedId.insert (id);
                      break;
                    } else {
                      patternsForUse.insert (patternsForUse.begin (), allPats.begin (), allPats.end ());
                      nextIdQueue.push (a->getOutput ()->getIDWithVersion ());
                    }
                  }
                }
              }
            }
          }
          
          idToPatterns[id].push_back (patternsForUse);
        } else {
          //For everything else (like assignment of JSON Literal etc.) 
          //consider everything
          idToPatterns.erase (id);
          fullyUsedId.insert (id);
        }
      } else if (dynamic_cast <Call*> (use) != nullptr) {
        idToPatterns.erase (id);
        fullyUsedId.insert (id);
      } else if (dynamic_cast <ConditionalBranch*> (use) != nullptr) {
        idToPatterns.erase (id);
        fullyUsedId.insert (id);
      } else if (dynamic_cast <Return*> (use) != nullptr) {
        idToPatterns.erase (id);
        fullyUsedId.insert (id);
      }
        else {
        std::cout << __FILE__ << ":" << __LINE__ << ":" << "Didn't consider this case " << typeid (*use).name () << std::endl;
      }
    }
  }
  
  for (auto id : idToPatterns) {
    std::vector<std::string> unionOfJSONs;
    for (auto patternVec : id.second) {
      std::stringstream newJSONObj;
      std::stringstream patternForJSONObj;
      //TODO: We are hardcoding ".input" here. Need to improve it.
        
      //Create a new object with only require fields and a pattern
      //to get only required fields.
      patternForJSONObj << ".input";
      newJSONObj << "{";
      for (int i = 0; i < patternVec.size (); i++) {
        Pattern* pattern = patternVec[i];
        if (dynamic_cast <KeyGetPattern*> (pattern) != nullptr) {
          newJSONObj << R"(\")" << ((KeyGetPattern*)pattern)->getKeyName () << R"(\")" << ": ";
        } else if (dynamic_cast <FieldGetPattern*> (pattern) != nullptr) {
          newJSONObj << R"(\")" << ((FieldGetPattern*)pattern)->getFieldName () << R"(\")" << ": ";
        }
        
        newJSONObj << "{";
        patternForJSONObj << pattern->convert ();
      }
      
      newJSONObj << patternForJSONObj.str ();
      for (int i = 0; i < patternVec.size (); i++) {
        newJSONObj << "}";
      }
      
      unionOfJSONs.push_back (newJSONObj.str ());
    }
    
    std::string finalString = "";
    for (int i = 0; i < unionOfJSONs.size () - 1 ; i++) {
      //For more than one use create a union of all fields.
      finalString = finalString + unionOfJSONs[i] + " * ";
    }
    
    finalString = finalString + unionOfJSONs[unionOfJSONs.size() - 1];
    requiredPatterns[id.first] = finalString;
  }
  
  program->setJSONKeyAnalysis (requiredPatterns);
}

void optimize (Program* program)
{
  livenessAnalysis (program);
  jsonLivenessAnalysis (program);
}

void convertToWhiskCommands (ComplexCommand& cmds, std::ostream& out, bool to_optimize, bool print_ssa)
{
  Program* program = convertToSSA (&cmds, print_ssa);
  if (to_optimize) {
    optimize (program);
  }
  std::vector <WhiskSequence*> seqs;
  WhiskProgram* p = (WhiskProgram*)program->convert (program, seqs);
  p->generateCommand (out);
}

int main () {}
