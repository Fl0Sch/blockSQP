/*
 * blockSQP -- Sequential quadratic programming for problems with
 *             block-diagonal Hessian matrix.
 * Copyright (C) 2012-2015 by Dennis Janka <dennis.janka@iwr.uni-heidelberg.de>
 *
 * Licensed under the zlib license. See LICENSE for more details.
 */

#ifndef BLOCKSQP_DEFS_H
#define BLOCKSQP_DEFS_H

#include "math.h"
#include "stdio.h"
#include <set>
#include <limits>

#include "qpOASES.hpp"

namespace blockSQP
{

typedef char PATHSTR[4096];

} // namespace blockSQP

#endif
