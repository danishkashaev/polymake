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
#include "polymake/vector"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

Matrix<Rational>
prism_coord(const Matrix<Rational>& V,
            int& n_vertices, int& n_vertices_out,
            const Set<int>& rays,
            const Rational& z, const Rational& z_prime)
{

   if (n_vertices==0) {
      n_vertices=V.rows();
      n_vertices_out=2*n_vertices-rays.size();
   }
   return rays.empty()
          ? Matrix<Rational>( ( V | same_element_vector(z, n_vertices) ) /
                              ( V | same_element_vector(z_prime, n_vertices)) )
          : Matrix<Rational>( ( V | same_element_sparse_vector(~rays, z, n_vertices) ) /
                              ( V.minor(~rays,All) | same_element_vector(z_prime, n_vertices-rays.size()) ) );
}


perl::Object prism(perl::Object p_in, const Rational& z, const Rational& z_prime, perl::OptionSet options)
{

   if(z==z_prime)
        throw std::runtime_error("prism: z and z' must be different");

   int n_vertices=0, n_vertices_out=0;
   Set<int> rays;
   if (!options["noc"])
      p_in.give("FAR_FACE") >> rays;

   perl::Object p_out("Polytope<Rational>");
   p_out.set_description() << "prism over " << p_in.name() << endl;

   if (options["noc"] || p_in.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
      n_vertices=VIF.cols();
      n_vertices_out=2*n_vertices-rays.size();

      Set<int> far_facet=accumulate(cols(VIF.minor(All,rays)), operations::mul());
      if (far_facet.size() > 1) // if size()>1 then FAR_FACE is not a facet
         far_facet.clear();

      const IncidenceMatrix<> VIF_out=
         rays.empty()
         ? IncidenceMatrix<>( (VIF | VIF)                               // side facet = original facet && its clone
                              / sequence(0,n_vertices)          // bottom facet = original polytope
                              / sequence(n_vertices,n_vertices) )       // top facet = cloned polytope
         : IncidenceMatrix<>( (VIF.minor(~far_facet,All) | VIF.minor(~far_facet, ~rays))        // rays aren't cloned
                              / sequence(0,n_vertices)          //   and belong to both (top&bottom) facets
                              / (rays + sequence(n_vertices,n_vertices-rays.size())) );

      p_out.take("N_VERTICES") << n_vertices_out;
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   if (options["noc"]) {
      if (p_in.exists("COMBINATORIAL_DIM")) {
         const int dim=p_in.give("COMBINATORIAL_DIM");
         p_out.take("COMBINATORIAL_DIM") << dim+1;
      }

   } else {
      const bool pointed=p_in.give("POINTED");
      if (!pointed)
         throw std::runtime_error("prism: input polyhedron not pointed");

      const Matrix<Rational> V=p_in.give("VERTICES");
      const Matrix<Rational> V_out=prism_coord(V, n_vertices, n_vertices_out, rays, z, z_prime);
      p_out.take("VERTICES") << V_out;
      const Matrix<Rational> empty;
      p_out.take("LINEALITY_SPACE") << empty;
   }

   if (options["relabel"]) {
      std::vector<std::string> labels(n_vertices_out);
      read_labels(p_in, "VERTEX_LABELS", labels);
      const std::string tick="'";
      copy(entire(attach_operation(select(labels, sequence(0,n_vertices)-rays),
                                   constant(tick), operations::add())),
           labels.begin()+n_vertices);
      p_out.take("VERTEX_LABELS") << labels;
   }

   return p_out;
}

UserFunction4perl("# @category  Producing a polytope from polytopes"
                  "# Make a prism over a pointed polyhedron."
                  "# The prism is the product of the input polytope //P// and the interval [//z1//, //z2//]."
                  "# @param Polytope P the input polytope"
                  "# @param Rational z1 the left endpoint of the interval; default value: -1"
                  "# @param Rational z2 the right endpoint of the interval; default value: -//z1//"
                  "# @option Bool noc only combinatorial information is handled"
                  "# @option Bool relabel creates an additional section [[VERTEX_LABELS]];"
                  "#   the bottom facet vertices get the labels from the original polytope;"
                  "#   the labels of their clones in the top facet get a tick (') appended."
                  "# @return Polytope",
                  &prism, "prism(Polytope; $=-1, $=-$_[1], { noc => undef, relabel => undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End: