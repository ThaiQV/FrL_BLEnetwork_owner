/**************************************************************************
 *    (C)2025 Freelux
 *
 *File 				: datat_types.h
 *Created on		: Apr 2025
 *Author			: Freelux RnD Team
 *Description		: define special type of variable or the struct.
 *License: Revised BSD License, see LICENSE.TXT file include in the project
 **************************************************************************/
#include "types.h"
#include "stdbool.h"
#include "math.h"

#define FL_BIT_IS_HIGH(parent, x)   ((BIT(x) & (parent)) != 0)

typedef struct
{
	u8 major;
	u8 minor;
	u8 patch;
}fl_version_t;
