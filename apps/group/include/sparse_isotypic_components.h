/* Copyright (c) 1997-2016
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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

#ifndef __GROUP_SPARSE_ISOTYPIC_COMPONENTS_H
#define __GROUP_SPARSE_ISOTYPIC_COMPONENTS_H

#include "polymake/group/group_tools.h"
#include "polymake/group/orbit.h"
#include "polymake/hash_set"
#include "polymake/linalg.h"
#include <fstream>

namespace polymake { namespace group {

template <typename SparseSet, typename NumericalType=Rational>
SparseIsotypicBasis<SparseSet>
sparse_isotypic_basis_impl(int order,
                           const Array<Array<int>>& original_generators,
                           const ConjugacyClasses& conjugacy_classes,
                           const Vector<Rational>& character,
                           const Array<SparseSet>& induced_orbit_representatives,
                           const std::string& filename = "")
{
   std::vector<SparseSimplexVector<SparseSet>> basis_hash_vectors;
   const Rational c0_ord(character[0] / order), one_half(1,2);
   std::ofstream outfile;
   if (filename != "" && filename != "-")
      outfile = std::ofstream(filename.c_str(), std::ios_base::trunc);
   std::ostream& os = (filename == "-" ? perl::cout : outfile);

   // for measured efficiency reasons, we once and for all allocate a SparseSet to receive permutations
   SparseSet working_set(induced_orbit_representatives[0]);
   working_set.clear();
   
   /*
     for each representative of an orbit of facets, we calculate the corresponding block     
         B = sum_{g in Gamma} chi_i(g) phi(g)
     of the projection matrix.
   */

   for (const auto& orep : induced_orbit_representatives) {
      // The rows and columns of B are indexed by the orbit of rep.
      const auto face_orbit(orbit<on_container, Array<int>, SparseSet>(original_generators, orep));
      
      // we need to index the orbit, and to create constant-time access to indexed elements
      hash_map<SparseSet, int> index_of;
      std::vector<SparseSet> face_orbit_indexed;
      face_orbit_indexed.reserve(face_orbit.size());
      int index(-1);
      for (const auto& f : face_orbit) {
         index_of[f] = ++index;
         face_orbit_indexed.push_back(f);
      }

      // We make an explicit ListMatrix to keep track of the linear span achieved so far. 
      // Whenever a new row is added to the ListMatrix, we append a corresponding SparseSimplexVector to basis_vectors.
      ListMatrix<SparseVector<NumericalType>>
         class_sparse_eqs(0, face_orbit.size()),
         kernel_so_far(unit_matrix<NumericalType>(face_orbit.size()));

      // for each potential new row of the block B, check if it is linearly independent from what is already there
      for (const auto& f : face_orbit) {
         /* 
            Each phi(g) is a permutation matrix.
            For example, for the permutation (b,a,c) we get the matrix
              a  b  c
                 1     a
              1        b
                    1  c
            Therefore, the row corresponding to f gets a contribution chi_i(g) in the column g(f), for all g in Gamma.
         */
         SparseVector<NumericalType> new_sparse_eq(face_orbit.size());
         for (int i=0; i<conjugacy_classes.size(); ++i) {
            if (is_zero(character[i])) 
               continue;
            for (const auto& g : conjugacy_classes[i]) {
               group::permute_to(f.begin(), g, working_set);
               new_sparse_eq[index_of[working_set]] += convert_to<NumericalType>(character[i]);
            }
         }
         
         if (add_row_if_rowspace_increases(class_sparse_eqs, new_sparse_eq, kernel_so_far)) {
            SparseSimplexVector<SparseSet> new_hash_eq;
            for (typename Entire<SparseVector<NumericalType>>::const_iterator eit = entire(new_sparse_eq); !eit.at_end(); ++eit) {
               // multiply by chi_i(id)/|Gamma|
               new_hash_eq[face_orbit_indexed[eit.index()]] = floor(convert_to<Rational>(*eit) + one_half) * c0_ord; 
            }
            if (filename.size())
               wrap(os) << new_hash_eq << endl;
            else
               basis_hash_vectors.push_back(new_hash_eq);
         }
      }
   }
   
   return SparseIsotypicBasis<SparseSet>(basis_hash_vectors);
}

template <typename SparseSet>
auto
sparse_isotypic_spanning_set_and_support_impl(int order,
                                              const Array<Array<int>>& original_generators,
                                              const ConjugacyClasses& conjugacy_classes,
                                              const Vector<Rational>& character,
                                              const Array<SparseSet>& induced_orbit_representatives,
                                              const std::string& filename = "",
                                              bool calculate_support = true)
{
   std::vector<SparseSimplexVector<SparseSet>> spanning_hash_vectors;
   hash_set<SparseSet> support;

   const Rational c0_ord(character[0] / order);
   std::ofstream outfile;
   if (filename != "" && filename != "-")
      outfile = std::ofstream(filename.c_str(), std::ios_base::trunc);
   std::ostream& os = (filename == "-" ? perl::cout : outfile);

   SparseSet working_set(induced_orbit_representatives[0]);
   working_set.clear();

   SparseSimplexVector<SparseSet> old_hash_eq;
   for (const auto& orep : induced_orbit_representatives) {
      for (const auto& f : orbit<on_container, Array<int>, SparseSet>(original_generators, orep)) {
         SparseSimplexVector<SparseSet> new_hash_eq;
         for (int i=0; i<conjugacy_classes.size(); ++i) {
            if (is_zero(character[i])) 
               continue;
            for (const auto& g : conjugacy_classes[i]) {
               group::permute_to(f.begin(), g, working_set);
               new_hash_eq[working_set] += character[i] * c0_ord;
            }
         }
         if (new_hash_eq == old_hash_eq) continue; // guard against the most trivial repetition
         old_hash_eq = new_hash_eq;
         if (calculate_support) {
            for (const auto m : old_hash_eq)
               if (!is_zero(m.second))
                  support += m.first;            
         } else  {
            if (filename.size())
               wrap(os) << new_hash_eq << endl;
            else
               spanning_hash_vectors.push_back(new_hash_eq);
         }
      }
   }
   
   if (calculate_support && filename.size())
      wrap(os) << support << endl;
   
   return std::make_pair(Array<SparseSimplexVector<SparseSet>>(spanning_hash_vectors.size(), entire(spanning_hash_vectors)), support);
}

template<typename SparseSet>
void
augment_index_of(hash_map<SparseSet, int>& index_of,
                 const SparseIsotypicBasis<SparseSet>& subspace_generators)
{
   int index(index_of.size());
   for (const auto& sgen : subspace_generators)
      for (const auto m : sgen)
         if (!index_of.exists(m.first))
            index_of[m.first] = index++;
}

template<typename SparseSet>
ListMatrix<SparseVector<Rational>>
list_matrix_representation(const hash_map<SparseSet, int>& index_of,
                           const SparseIsotypicBasis<SparseSet>& subspace_generators)
{
  ListMatrix<SparseVector<Rational>> sgen_matrix(0, index_of.size());
   for (const auto& sgen : subspace_generators) {
      SparseVector<Rational> new_sgen(index_of.size());
      for (const auto m : sgen)
         new_sgen[index_of.at(m.first)] = m.second;
      sgen_matrix /= new_sgen;
   }
   return sgen_matrix;
}

template<typename SparseSet>
bool
spans_invariant_subspace_impl(const Array<Array<int>>& group_generators,
                              const SparseIsotypicBasis<SparseSet>& subspace_generators,
                              bool verbose)
{
   hash_map<SparseSet,int> index_of;
   augment_index_of(index_of, subspace_generators);
   const SparseMatrix<Rational> ker = null_space(list_matrix_representation(index_of, subspace_generators));

   for (const auto& sgen : subspace_generators) {
      // this is a kludge (part 1) because making hash_sets or Sets of hash_maps doesn't call the right equality comparison operator
      Map<SparseSet, Rational> copy_map;
      for (const auto v : sgen)
         copy_map[v.first] = v.second;

      // kludge part 2. Ideally, this should read orbit<on_container>(group_generators, sgen)
      for (const auto& o_sgen : orbit<on_container, Array<int>, Map<SparseSet,Rational>, Set<Map<SparseSet,Rational>>>(group_generators, copy_map)) {
         //      for (const auto& o_sgen : orbit<on_container>(group_generators, sgen)) {
         //         typedef typename pm::deref<decltype(sgen)>::type GenType;

         SparseVector<Rational> new_sgen(index_of.size());
         for (const auto m : o_sgen) {
            try {
               new_sgen[index_of.at(m.first)] = m.second;
            } catch (no_match) {
               if (verbose) cerr << "The given vectors do not span an invariant subspace, because "
                                 << m << " is in the support of the orbit of " << sgen
                                 << ", but not in the orbit of the support of the given vectors" << endl;
               return false;
            }
         }
         if (!is_zero(ker * new_sgen)) {
            if (verbose) cerr << "The given vectors do not span an invariant subspace, because "
                              << new_sgen << ", corresponding to " 
                              << o_sgen << " is not in the spanned subspace L. Here, ker L =\n"
                              << ker << endl;
            return false;
         }
      }
   }
   return true;
}

} }

#endif // __GROUP_SPARSE_ISOTYPIC_COMPONENTS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
