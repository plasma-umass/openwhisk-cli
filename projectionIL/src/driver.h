#include "ast.h"

#ifndef __DRIVER_H__
#define __DRIVER_H__

class Converter
{
public:
  static WhiskSequence* convert(Command* cmd);
};

#endif /*__DRIVER_H__*/
