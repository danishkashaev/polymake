/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/Map.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/list"
#include <sstream>

namespace polymake { namespace topaz {

namespace {

void convert_labels(const Array<std::string>& string_labels, Array<Set<Set<Int>>>& labels_as_set)
{
   auto lsit = entire(labels_as_set);
   for (auto lit = entire(string_labels); !lit.at_end(); ++lit) {
      std::istringstream is(*lit);
      is.ignore(1); // "{"
      Set<Set<Int>> labels;
      while (is.good() && is.peek() != '}') {
         is.ignore(1); // '{'
         while (is.good() && is.peek() != '}') {
            Set<Int> new_set;
            while (is.good() && is.peek() != '}') {
               if (is.peek() == '{') is.ignore(1); 
               Int i;
               is >> i;
               new_set += i;
               if (is.peek() == ',' || is.peek() == ' ') is.ignore(1); 
            }
            labels += new_set;
            if (is.peek() == '}') is.ignore(1); 
            if (is.peek() == ',' || is.peek() == ' ') is.ignore(1); 
         }
      }
      *lsit++ = labels;
   }
}

bool on_boundary(const Set<Set<Int>>& label, Int d, const IncidenceMatrix<>& VIF)
{
   Set<Int> face;
   for (auto lit = entire(label); !lit.at_end(); ++lit)
      face += *lit;
   for (auto rit = entire(rows(VIF)); !rit.at_end(); ++rit) 
      if (!(face - *rit).size()) return true; // it's contained in the boundary
   return false;
}

void identify_labels(Int d, const group::PermlibGroup& identification_group, const IncidenceMatrix<>& VIF, Array<Set<Set<Int>>>& labels_as_set)
{
   for (auto lit = entire(labels_as_set); !lit.at_end(); ++lit)
      if (on_boundary(*lit, d, VIF)) 
         *lit = *(identification_group.orbit(*lit).begin());
}

} // end anonymous namespace

BigObject bs2quotient(BigObject p, BigObject bs)
{
   const Array<Array<Int>> generators = p.give("QUOTIENT_SPACE.IDENTIFICATION_ACTION.GENERATORS");
   const group::PermlibGroup identification_group(generators);
   const IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
   const Array<std::string> labels = bs.give("VERTEX_LABELS");
   const Int n = labels.size();
   const Array<Set<Int>> facets = bs.give("FACETS");
   if (!facets.size() || !facets[0].size()) throw std::runtime_error("Got no facets");
   const Int d = facets[0].size()-1;
   
   Array<Set<Set<Int>>> labels_as_set(n);
   convert_labels(labels, labels_as_set);
   identify_labels(d, identification_group, VIF, labels_as_set);

   std::vector<std::string> identified_labels;
   Map<Set<Set<Int>>, Int> index_of;
   Int index = 0;
   std::ostringstream os;
   for (const auto& lset : labels_as_set) {
      if (!index_of.contains(lset)) {
         index_of[lset] = index++;
         wrap(os) << lset;
         identified_labels.push_back(os.str());
         os.str("");
      }
   }

   Set<Set<Int>> identified_facets;
   for (const auto& f : facets) {
      Set<Int> new_facet;
      for (const auto& s : f)
         new_facet += index_of[labels_as_set[s]];
      identified_facets += new_facet;
   }

   return BigObject("topaz::SimplicialComplex",
                    "FACETS", identified_facets,
                    "VERTEX_LABELS", identified_labels,
                    "PURE", true,
                    "DIM", d);
}

InsertEmbeddedRule("REQUIRE_APPLICATION polytope\n\n");

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Create a simplicial complex from a simplicial subdivision of a given complex"
                  "# by identifying vertices on the boundary of the original complex according to a group that acts on vertices." 
                  "# @param polytope::Polytope P the underlying polytope"
                  "# @param SimplicialComplex complex a sufficiently fine subdivision of P, for example the second barycentric subdivision"
                  "# @return SimplicialComplex",
                  &bs2quotient,
                  "bs2quotient(polytope::Polytope SimplicialComplex)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
