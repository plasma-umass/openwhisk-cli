#include <string>

#ifndef __WHISK_ACTION_H__
#define __WHISK_ACTION_H__

#define WHISK_FORK_NAME_LENGTH 10

class WhiskAction
{
private:
  std::string name;
  
public:
  WhiskAction(std::string _name) : name(_name)
  {
  }
  
  virtual ~WhiskAction () {}
};

class WhiskSequence : public WhiskAction
{
private:
  vector<WhiskAction*> actions;
public:
  WhiskSequence(std::string name) : WhiskSequence (name)
  {
  }
  
  WhiskSequence(std::string name, vector<WhiskAction*> _actions) : WhiskAction(name), actions(_actions)
  {
  }
  
  vector<WhiskAction*>& getActions () {return actions;}
  void appendAction (WhiskAction* action) {actions.push_back (action);}
  void insertAction (WhiskAction* action, int index) {actions.insert (index, action);}
}

class WhiskProjection : public WhiskAction
{
private:
  std::string projCode;

public:
  WhiskProjection (std::string name, std::string _code) : WhiskAction (name), projCode (_code)
  {
  }
  
  std::string getProjCode () {return projCode;}
};

class WhiskFork : public WhiskAction
{
private:
  std::string innerActionName;
  WhiskAction* innerAction;

public:
  WhiskFork (std::string name, std::string _innerActionName) : WhiskAction(name), innerActionName(_innerActionName)
  {
  }
  
  WhiskFork (std::string name, WhiskAction* innerAction) : WhiskAction(name), innerAction(_innerAction)
  {
  }
  
  std::string getInnerActionName() {return innerActionName;}
  WhiskAction* getInnerAction() {return innerAction;}
};

#endif
