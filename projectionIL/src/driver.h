#include <vector>

#include "ast.h"
#include "whisk_action.h"

#ifndef __DRIVER_H__
#define __DRIVER_H__

class Converter
{
public:
  static std::vector<WhiskSequence*> convert(Command* cmd);
};

#endif /*__DRIVER_H__*/
