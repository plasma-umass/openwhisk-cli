#include <string>
#include <vector>
#include <string.h>
#include "utils.h"
#include "serverless.h"

#ifndef __WHISK_ACTION_H__
#define __WHISK_ACTION_H__

#define WHISK_CLI_PATH "wsk"
#define WHISK_CLI_ARGS "-i"
#define ECHO(x) (std::string("echo \"")+x+"\"")
enum
{
  WHISK_FORK_NAME_LENGTH = 10,
  WHISK_SEQ_NAME_LENGTH = 10,
  WHISK_PROJ_NAME_LENGTH = 10
};

typedef ServerlessAction WhiskAction;

class WhiskSequence : public ServerlessSequence
{
public:
  WhiskSequence(std::string name) : ServerlessSequence (name)
  {
  }
  
  WhiskSequence(std::string name, std::vector<ServerlessAction*> _actions) : ServerlessSequence(name, _actions)
  {
  }
  
  virtual void print ()
  {
    fprintf (stdout, "(WhiskSequence %s, %ld, (", getName (), actions.size ());
    
    for (auto action : actions) {
      action->print ();
      fprintf (stdout, " -> ");
    } 
    
    fprintf (stdout, "))\n");
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    for (auto action : actions) {
      action->generateCommand (os);
    }
    
    os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action update " << getName () << " --sequence ";
    if (actions.size () > 0) {
      for (int i = 0; i < actions.size () - 1; i++) {
        os << actions[i]->getNameForSeq () << ",";
      }
      
      os << actions[actions.size () - 1]->getNameForSeq () << std::endl;
    }
  }
};

class WhiskProjection : public ServerlessProjection
{
public:

  WhiskProjection (std::string name, std::string _code) : ServerlessProjection (name, _code) 
  {
  }

  virtual void generateCommand(std::ostream& os)
  {
    char temp[256];
    assert (getProjectionTempFile (temp, 256) != -1);
    os << ECHO(getProjCode ()) << " > " << temp << "\n";
    os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action update " << getName () << " --projection " << temp << "\n";
  }
};

class WhiskFork : public ServerlessFork
{
public:
  WhiskFork (std::string name, std::string _innerActionName, std::string _returnName, std::string requiredFields) : ServerlessFork (name, _innerActionName, _returnName, requiredFields)
  {
  }
  
  WhiskFork (std::string name, ServerlessAction* _innerAction, std::string _returnName, std::string requiredFields) : ServerlessFork (name, _innerAction, _returnName, requiredFields)
  {
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action update " <<
      getName() << " --fork " << getInnerActionName() << std::endl;
    
    char temp[256];
    assert (getProjectionTempFile (temp, 256) != -1);
    char code[2048];
    resultProjectionName = "Proj_"+gen_random_str (WHISK_PROJ_NAME_LENGTH);
    if (requiredFields != "") {
      sprintf (code, R"(. * {\"saved\": {\"%s\": %s}})", returnName.c_str(), requiredFields.c_str ());
    } else {
      sprintf (code, R"(. * {\"saved\": {\"%s\": .input}})", returnName.c_str());
    }
    os << ECHO(code) << " > " << temp << std::endl;
    os << WHISK_CLI_PATH << " " WHISK_CLI_ARGS << " action update " << 
       resultProjectionName << " --projection " << temp << std::endl;
  }
};

class WhiskProjForkPair : public ServerlessAction
{
private:
  WhiskFork* fork;
  WhiskProjection* proj;

public:
  WhiskProjForkPair (WhiskProjection* _proj, WhiskFork* _fork):
    WhiskAction ("ProjForkPair_" + gen_random_str (WHISK_FORK_NAME_LENGTH)),
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
    fork->generateCommand(os);
  }
  
  virtual std::string getNameForSeq () {return proj->getName () + std::string(",") + fork->getName () + "," + fork->getResultProjectionName();}
};

typedef ServerlessApp WhiskApp;

class WhiskDirectBranch : public ServerlessApp
{
private:
  std::string target;
  WhiskProjection* proj;
  
public:
  WhiskDirectBranch (std::string _target) : target(_target)
  {
    proj = new WhiskProjection ("Proj_DirectBranch_" +gen_random_str (WHISK_PROJ_NAME_LENGTH), 
                       R"(. * {\"action\":\")" + target+R"(\"})"); //TODO: Wrap correctly in app.
    
  }
  
  virtual void print ()
  {
    fprintf (stdout, "App (%s)", target.c_str ());
  }
  
  virtual void generateCommand (std::ostream& os)
  {
    proj->generateCommand (os);
    //os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action invoke " << getName () << std::endl;
  }
  
  virtual std::string getNameForSeq () {return std::string(proj->getName ());}
};

class WhiskProgram : public ServerlessProgram 
{
public:
  WhiskProgram (std::string _name) : ServerlessProgram (_name)
  {
  }
  
  WhiskProgram (std::string _name, std::vector <WhiskSequence*> _basicBlocks) : 
    ServerlessProgram (_name, std::vector<ServerlessSequence*> (_basicBlocks.begin(), _basicBlocks.end()))
  {
  }
  
  virtual void generateCommand(std::ostream& os)
  {
    for (auto block : basicBlocks) {
      block->generateCommand (os);
      std::cout << std::endl;
    }
    
    os << WHISK_CLI_PATH << " " << WHISK_CLI_ARGS << " action update " << getName () << " --program ";
    if (basicBlocks.size () > 0) {
      for (int i = 0; i < basicBlocks.size () - 1; i++) {
        os << basicBlocks[i]->getNameForSeq () << ",";
      }
      
      os << basicBlocks[basicBlocks.size () - 1]->getNameForSeq () << std::endl;
    }
  }
  
  virtual void print ()
  {
    fprintf (stdout, "(WhiskProgram %s, %ld, (", getName (), basicBlocks.size ());
    
    for (auto block : basicBlocks) {
      block->print ();
      fprintf (stdout, " -> ");
    } 
    
    fprintf (stdout, "))\n");
  }
};
#endif
