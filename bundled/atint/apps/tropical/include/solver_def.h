/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	This file contains a simple using clause to select the convex hull
	used in a-tint. This way, we can switch the algorithm we use by
	changing only a single file (this one).

*/


#ifndef POLYMAKE_ATINT_SOLVER_DEF_H
#define POLYMAKE_ATINT_SOLVER_DEF_H

#include "polymake/polytope/cdd_interface.h"

namespace polymake { namespace tropical {

	//Change this line! (And make sure you include the right headers)
	using polymake::polytope::cdd_interface::solver;

}}

#endif