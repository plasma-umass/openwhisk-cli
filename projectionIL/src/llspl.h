#include <string>
#include <vector>
#include <string.h>
#include "utils.h"
#include "serverless.h"

#ifndef __LLSPL_H__
#define __LLSPL_H__

typedef ServerlessAction LLSPLAction;

class LLSPLSequence : public ServerlessSequence
{
public:
  LLSPLSequence(std::string name) : ServerlessSequence (name)
  {
  }
  
  LLSPLSequence(std::string name, std::vector<ServerlessAction*> _actions) : ServerlessSequence(name, _actions)
  {
  }
  
  virtual void print ()
  {
    fprintf (stdout, "(LLSPLSequence %s, %ld, (", getName (), actions.size ());
    
    for (auto action : actions) {
      action->print ();
      fprintf (stdout, " -> ");
    } 
    
    fprintf (stdout, "))\n");
  }
  
  virtual void generateCommand(std::ostream& os)
  {    
    os << "(";
    if (actions.size () > 0) {
      for (int i = 0; i < actions.size () - 1; i++) {
        actions[i]->generateCommand (os);
        os << " -> ";
      }
      
      actions[actions.size () - 1]->generateCommand (os);
      os << ")";
    }
  }
};

class LLSPLProjection : public ServerlessProjection
{
public:

  LLSPLProjection (std::string name, std::string _code) : ServerlessProjection (name, _code) 
  {
  }

  virtual void generateCommand(std::ostream& os)
  {
    os << "JsonTransform ("<<getProjCode () << ")";
  }
};

class LLSPLDirectBranch : public ServerlessApp
{
private:
  std::string target;
  LLSPLProjection* proj;
  
public:
  LLSPLDirectBranch (std::string _target) : target(_target)
  {
    //proj = new LLSPLProjection ("Proj_DirectBranch_" +gen_random_str (WHISK_PROJ_NAME_LENGTH), 
    //                   R"(. * {\"action\":\")" + target+R"(\"})"); //TODO: Wrap correctly in app.
    
  }
  
  virtual void print ()
  {
    fprintf (stdout, "App (%s)", target.c_str ());
  }
  
  virtual void generateCommand (std::ostream& os)
  {
    //proj->generateCommand (os);
    //os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action invoke " << getName () << std::endl;
  }
  
  virtual std::string getNameForSeq () {return std::string(proj->getName ());}
};


class LLSPLFork : public ServerlessFork
{
public:
  LLSPLFork (std::string name, std::string _innerActionName, std::string _returnName) : ServerlessFork (name, _innerActionName, _returnName, "")
  {
  }
  
  LLSPLFork (std::string name, ServerlessAction* _innerAction, std::string _returnName) : ServerlessFork (name, _innerAction, _returnName, "")
  {
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    os << "Split (" << getInnerActionName() << ")";
    
    /*char temp[256];
    assert (getProjectionTempFile (temp, 256) != -1);
    char code[2048];
    resultProjectionName = "Proj_"+gen_random_str (WHISK_PROJ_NAME_LENGTH);
    sprintf (code, R"(. * {\"saved\": {\"%s\": .input}})", returnName.c_str());
    os << ECHO(code) << " > " << temp << std::endl;
    os << WHISK_CLI_PATH << " " WHISK_CLI_ARGS << " action update " << 
       resultProjectionName << " --projection " << temp << std::endl;
       */
  }
};

class LLSPLProjForkPair : public ServerlessAction
{
private:
  LLSPLFork* fork;
  LLSPLProjection* proj;

public:
  LLSPLProjForkPair (LLSPLProjection* _proj, LLSPLFork* _fork):
    LLSPLAction ("ProjForkPair_" + gen_random_str (WHISK_FORK_NAME_LENGTH)),
    fork(_fork), proj(_proj)
  {
  }
  
  virtual void print ()
  {
    proj->print ();
    fprintf (stdout, " -> ");
    fork->print ();
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    proj->generateCommand(os);
    os << " -> ";
    fork->generateCommand(os);
  }
  
  virtual std::string getNameForSeq () {return proj->getName () + std::string(",") + fork->getName () + "," + fork->getResultProjectionName();}
};

class LLSPLIf : public ServerlessAction
{
protected:
  std::string cond;
  LLSPLAction* thenAction;
  LLSPLAction* elseAction;
  
public:
  LLSPLIf (std::string _name, std::string _cond, LLSPLAction* _thenAction, LLSPLAction* _elseAction) : 
    ServerlessAction (_name), cond(_cond), thenAction(_thenAction), elseAction(_elseAction)
  {
  }
  
  virtual void print ()
  {
    fprintf (stdout, "If (");
    //cond->print ();
    std::cout << cond;
    fprintf (stdout, ", ");
    thenAction->print ();
    fprintf (stdout, ", ");
    elseAction->print ();
    fprintf (stdout, ")");
  }
  
  virtual void generateCommand (std::ostream& os)
  {
    os << "If (" << cond;
    //cond->generateCommand (os);
    os << ", ";
    thenAction->generateCommand (os);
    os << ", ";
    elseAction->generateCommand (os);
    os << ")";
  }
};

class LLSPLProgram : public ServerlessProgram 
{
public:
  LLSPLProgram (std::string _name) : ServerlessProgram (_name)
  {
  }
  
  LLSPLProgram (std::string _name, std::vector <LLSPLSequence*> _basicBlocks) : 
    ServerlessProgram (_name, std::vector<ServerlessSequence*> (_basicBlocks.begin(), _basicBlocks.end()))
  {
  }
  
  virtual void generateCommand(std::ostream& os)
  {    
    if (basicBlocks.size () > 0) {
      for (int i = 0; i < basicBlocks.size () - 1; i++) {
        basicBlocks[i]->generateCommand (os);
        os << " >> ";
      }
      
      basicBlocks[basicBlocks.size () - 1]->generateCommand (os);
      os << std::endl;
    }
  }
  
  virtual void print ()
  {
    fprintf (stdout, "(LLSPLProgram %s, %ld, (", getName (), basicBlocks.size ());
    
    for (auto block : basicBlocks) {
      block->print ();
      fprintf (stdout, " -> ");
    } 
    
    fprintf (stdout, "))\n");
  }
};

#endif
