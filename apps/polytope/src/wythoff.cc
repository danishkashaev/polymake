/* Copyright (c) 1997-2014
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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Set.h"
#include "polymake/hash_set"
#include "polymake/Map.h"
#include "polymake/Array.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/ListMatrix.h"
#include "polymake/polytope/simple_roots.h"
#include "polymake/linalg.h"
#include "polymake/group/group_domain.h"
#include <sstream>
#include <vector>
#include <list>

namespace polymake { namespace polytope {

namespace {

template <typename E>
SparseVector<E> find_initial_point(const Set<int>& rings, const SparseMatrix<E>& R, int d, bool is_type_A)
{
   SparseMatrix<E> equations (R);
   for (Entire<Set<int> >::const_iterator sit = entire(rings); !sit.at_end(); ++sit)
      equations(*sit, 0) = E(-1); // we look for a point not on any hyperplane indexed by rings, but on all others
   if (is_type_A)
      equations /= SparseVector<E> (E(0) | ones_vector<E>(d));
   SparseVector<E> p0(null_space(equations)[0]);
   p0.dehomogenize();
   return p0;
}

template <typename E>
SparseMatrix<E> orbit (const SparseVector<E>& p0, const SparseMatrix<E>& R, Map<SparseVector<E>,int>& index_of)
{
   hash_set<SparseVector<E> > 
      points,
      roots;
   std::vector<SparseVector<E> > 
      ordered_points,
      ordered_roots; 
   int index(0);
   points += p0;
   ordered_points.push_back(p0);
   index_of[p0] = index++;
   for (typename Entire<Rows<SparseMatrix<E> > >::const_iterator rit = entire(rows(R)); !rit.at_end(); ++rit) {
      roots += *rit;
      ordered_roots.push_back(*rit);
   }

   typedef std::pair<unsigned int, unsigned int> key_type; // (root, point) not yet reflected
   std::list<key_type> 
      point_queue, 
      root_queue;  
   for (int i=0; i<R.rows(); ++i)
      point_queue.push_back(key_type(i,0)); 
   for (int i=0; i<R.rows()-1; ++i)
      for (int j=i+1; j<R.rows(); ++j)
         root_queue.push_back(key_type(i,j)); 

   bool done (point_queue.empty() && root_queue.empty());
   while (!done) {
      if (!point_queue.empty()) {
         const key_type a(point_queue.front());  point_queue.pop_front();
         const SparseVector<E> new_point(reflect(ordered_points[a.second], ordered_roots[a.first]));
         if (!points.exists(new_point)) {
            for (unsigned int i=0; i<roots.size(); ++i)
               point_queue.push_back(key_type(i, points.size()));
            points += new_point;
            ordered_points.push_back(new_point);
            index_of[new_point] = index++;
         }
      }
      if (!root_queue.empty()) {
         const key_type r(root_queue.front());  root_queue.pop_front();
         const SparseVector<E> new_root(reflect(ordered_roots[r.second], ordered_roots[r.first]));
         if (!roots.exists(new_root) && !roots.exists(-new_root)) {
            for (unsigned int i=0; i<roots.size(); ++i)
               root_queue.push_back(key_type(i, roots.size()));
            for (unsigned int j=0; j<points.size(); ++j)
               point_queue.push_back(key_type(roots.size(), j));
            roots += new_root;
            ordered_roots.push_back(new_root);
         }
      }
      done = point_queue.empty() && root_queue.empty();
   }
   return SparseMatrix<E>(ordered_points.size(), ordered_points[0].dim(), entire(ordered_points)); 
}

} // end anonymous namespace

template <typename E>
void wythoff(const std::string& type, const Set<int>& rings, const SparseMatrix<E>& R, SparseMatrix<E>& V, Array<Array<int> >& generators)
{
   if (accumulate(rings, operations::max()) >= R.rows())
      throw std::runtime_error("Set specifies non-existing rows of the root matrix");
   const int d = R.cols()-1;

   const SparseVector<E> p0 = find_initial_point(rings, R, d, type[0] == 'A' || type[0] == 'a');
   Map<SparseVector<E>,int> index_of;
   V = orbit(p0, R, index_of);
   generators = Array<Array<int> >(R.rows());
   for (int i=0; i<R.rows(); ++i) {
      Array<int> g(V.rows());
      for (int j=0; j<V.rows(); ++j) {
         const SparseVector<E> r(R.row(i)), p(V.row(j));
         g[j] = index_of[reflect(p, r)];
      }
      generators[i] = g;
   }
}

perl::Object wythoff_dispatcher(std::string type, Set<int> rings)
{
   if (type.size() < 2)
      throw std::runtime_error("Type needs single letter followed by rank.");

   const char t(type[0]);
   int n;
   std::istringstream is (type.substr(1));
   is >> n;

   perl::Object p;
   Array<Array<int> > generators;

   typedef QuadraticExtension<Rational> QE;

   if ((t >= 'A' && t <= 'G') || (t >= 'a' && t <= 'g')) {
      SparseMatrix<Rational> RM;
      if (t == 'A' || t == 'a') {
         if (n >= 1)
            wythoff<Rational>(type, rings, simple_roots_type_A(n), RM, generators);
         else
            throw std::runtime_error("Coxeter group of type A requires rank >= 1.");
      } else if (t == 'B' || t == 'b') {
         if (n >= 2)
            wythoff<Rational>(type, rings, simple_roots_type_B(n), RM, generators);
         else
            throw std::runtime_error("Coxeter group of type B requires rank >= 2.");
      } else if (t == 'C' || t == 'c') {
         if (n >= 2)
            wythoff<Rational>(type, rings, simple_roots_type_C(n), RM, generators);
         else
            throw std::runtime_error("Coxeter group of type C requires rank >= 2.");
      } else if (t == 'D' || t == 'd') {
         if (n >= 3)
            wythoff<Rational>(type, rings, simple_roots_type_D(n), RM, generators);
         else 
            throw std::runtime_error("Coxeter group of type D requires rank >= 3.");
      } else if (t == 'E' || t == 'e') {
         if (n==8)
            wythoff<Rational>(type, rings, simple_roots_type_E8(), RM, generators);
         else throw std::runtime_error("Coxeter groups E6 and E7 not implemented.");
      }
      else if (t == 'F' || t == 'f') {
         if (n == 4)
            wythoff<Rational>(type, rings, simple_roots_type_F4(), RM, generators);
         else throw std::runtime_error("Coxeter group of type F requires rank == 4.");
      } else if (t == 'G' || t == 'g') {
         if (n == 2)
            wythoff<Rational>(type, rings, simple_roots_type_G2(), RM, generators);
         else throw std::runtime_error("Coxeter group of type G requires rank == 2.");
      } else
         throw std::runtime_error("Did not recognize crystallographic Coxeter group.");

      p = perl::Object("Polytope<Rational>");
      p.take("VERTICES") << RM;
      p.take("N_VERTICES") << RM.rows();
      p.take("LINEALITY_SPACE") << Matrix<Rational>();
   } else if (t == 'H' || t == 'h') {
      SparseMatrix<QE> EM;
      switch(n) {
      case 3: {
         wythoff<QE>(type, rings, simple_roots_type_H3(), EM, generators);
         break;
      }
      case 4: {
         wythoff<QE>(type, rings, simple_roots_type_H4(), EM, generators);
         break;
      }
      default:
         throw std::runtime_error("Coxeter group of type H requires rank 3 or 4.");
      }

      p = perl::Object("Polytope<QuadraticExtension>");
      p.take("VERTICES") << EM;
      p.take("N_VERTICES") << EM.rows();
      p.take("LINEALITY_SPACE") << Matrix<QE>();
   } else
      throw std::runtime_error("Did not recognize the Coxeter group.");

   p.take("CONE_AMBIENT_DIM") << ((t == 'A' || t == 'a') ? n+2 : n+1);
   p.take("CONE_DIM") << n+1;
   p.take("AFFINE_HULL") << ((t == 'A' || t == 'a') ? vector2row(-1 | ones_vector<Rational>(n+1)) : Matrix<Rational>());
   p.take("BOUNDED") << true;
   p.take("FEASIBLE") << true;
   p.take("POINTED") << true;
   p.take("CENTERED") << true;

   perl::Object g("group::GroupOfPolytope");
   g.take("DOMAIN") << polymake::group::OnRays;
   g.take("GENERATORS") << generators;
   p.take("GROUP") << g;

   return p;
}

// wythoff_dispatcher("A3", Set<int>(0)) gives tetrahedron, but embedded in 4-space
// That's why we explictly give a full-dimensional representation here.

perl::Object tetrahedron() {
   Matrix<Rational> RM(same_element_matrix(1,4,4));
   RM(0,2) = RM(0,3) = RM(1,1) = RM(1,3) = RM(2,1) = RM(2,2) = -1;

   perl::Object p("Polytope<Rational>");
   p.take("VERTICES") << RM;
   p.take("N_VERTICES") << 4;
   p.take("LINEALITY_SPACE") << Matrix<Rational>();

   p.take("CONE_AMBIENT_DIM") << 4;
   p.take("CONE_DIM") << 4;
   p.take("BOUNDED") << true;
   p.take("FEASIBLE") << true;
   p.take("POINTED") << true;
   p.take("CENTERED") << true;

   return p;
}

// wythoff_dispatcher("B3", Set<int>(0)) gives octahedron

perl::Object cuboctahedron() {
   return wythoff_dispatcher("B3", Set<int>(1));
}

// wythoff_dispatcher("B3", Set<int>(2)) gives cube

perl::Object truncated_cuboctahedron() {
   return wythoff_dispatcher("B3", sequence(0,2));
}

perl::Object regular_24_cell() {
   return wythoff_dispatcher("F4", Set<int>(0));
}

perl::Object regular_120_cell() {
   return wythoff_dispatcher("H4", Set<int>(0));
}

perl::Object regular_600_cell() {
   return wythoff_dispatcher("H4", Set<int>(3));
}

perl::Object dodecahedron() {
   return wythoff_dispatcher("H3", Set<int>(0));
}

perl::Object icosidodecahedron() {
   return wythoff_dispatcher("H3", Set<int>(1));
}

perl::Object icosahedron() {
   return wythoff_dispatcher("H3", Set<int>(2));
}

perl::Object truncated_dodecahedron() {
   return wythoff_dispatcher("H3", sequence(0,2));
}

perl::Object rhombicosidodecahedron() {
   Set<int> rings;
   rings.push_back(0); rings.push_back(2);
   return wythoff_dispatcher("H3", rings);
}

perl::Object truncated_icosahedron() {
   return wythoff_dispatcher("H3", sequence(1,2));
}

perl::Object truncated_icosidodecahedron() {
   return wythoff_dispatcher("H3", sequence(0,3));
}


UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the orbit polytope of a point under a Coxeter arrangement"
                  "# with exact coordinates, possibly in a qudratic extension field of the rationals"
                  "# @param String   type   single letter followed by rank representing the type of the arrangement"
                  "# @param Set<Int> rings  indices of the hyperplanes corresponding to simple roots of the arrangement"
		  "# that the initial point should NOT lie on"
                  "# @return Polytope",
                  &wythoff_dispatcher, "wythoff($ Set<Int>)");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create regular tetrahedron.  A Platonic solid."
                  "# @return Polytope",
                  &tetrahedron, "tetrahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create cuboctahedron.  An Archimedean solid."
                  "# @return Polytope",
                  &cuboctahedron, "cuboctahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create truncated cuboctahedron.  An Archimedean solid."
                  "# Also known as the 3-permutahedron."
                  "# @return Polytope",
                  &truncated_cuboctahedron, "truncated_cuboctahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create regular 24-cell."
                  "# @return Polytope",
                  &regular_24_cell, "regular_24_cell()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact regular 120-cell in Q(sqrt{5})."
                  "# @return Polytope",
                  &regular_120_cell, "regular_120_cell()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact regular 600-cell in Q(sqrt{5})."
                  "# @return Polytope",
                  &regular_600_cell, "regular_600_cell()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact regular dodecahedron in Q(sqrt{5}).  A Platonic solid."
                  "# @return Polytope",
                  &dodecahedron, "dodecahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact icosidodecahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# @return Polytope",
                  &icosidodecahedron, "icosidodecahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact regular icosahedron in Q(sqrt{5}).  A Platonic solid."
                  "# @return Polytope",
                  &icosahedron, "icosahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact truncated dodecahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# @return Polytope",
                  &truncated_dodecahedron, "truncated_dodecahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact rhombicosidodecahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# @return Polytope",
                  &rhombicosidodecahedron, "rhombicosidodecahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact truncated icosahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# Also known as the soccer ball."
                  "# @return Polytope",
                  &truncated_icosahedron, "truncated_icosahedron()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact truncated icosidodecahedron in Q(sqrt{5}).  An Archimedean solid."
                  "# @return Polytope",
                  &truncated_icosidodecahedron, "truncated_icosidodecahedron()");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End: