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
#include "polymake/ListMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/hash_map"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace fan {

template <typename Coord>
perl::Object common_refinement(perl::Object f1, perl::Object f2)
{
   const int d=f1.give("FAN_DIM");
   const Array<Set<int> > max_cones1=f1.give("MAXIMAL_CONES");
   Matrix<Coord> rays1=f1.give("RAYS");
   const Matrix<Coord> lineality_space1=f1.give("LINEALITY_SPACE");
   const Matrix<Coord> lineality_space2=f2.give("LINEALITY_SPACE");
   const Matrix<Coord> lineality_space =
      null_space(null_space(lineality_space1) / null_space(lineality_space2));
   const Array<Set<int> > max_cones2=f2.give("MAXIMAL_CONES");
   Matrix<Coord> rays2=f2.give("RAYS");
   const bool complete = f1.give("COMPLETE") && f2.give("COMPLETE");
   project_to_orthogonal_complement(rays1, lineality_space1);
   project_to_orthogonal_complement(rays2, lineality_space2);


   ListMatrix<Vector<Coord> > rays;//=rays1;
   hash_map<Vector<Coord>, int> ray_map;
   /*
   int index=0;
   for (typename Entire<Rows<ListMatrix<Vector<Coord> > > >::const_iterator i=entire(rows(rays)); !i.at_end(); ++i)
      ray_map[*i]=index++;
   for (typename Entire<Rows<Matrix<Coord> > >::const_iterator i=entire(rows(rays2)); !i.at_end(); ++i){
      const Vector<Coord> ray=*i;
      const typename hash_map<Vector<Coord>,int>::iterator rep=ray_map.find(ray);
      if (rep==ray_map.end()) {
         ray_map[ray]=rays.rows();
         rays/=ray;
      }
   }
   */
   std::list<Set<int> > new_max_cones;
   perl::ObjectType cone_type=perl::ObjectType::construct<Coord>("Cone");

   Array<perl::Object> all_cones1(max_cones1.size());
   for (int i=0; i<max_cones1.size(); ++i) {
      all_cones1[i].create_new(cone_type);
      const Matrix<Coord> p1_cone_vert=rays1.minor(max_cones1[i],All);
      all_cones1[i].take("RAYS")<<p1_cone_vert;
      all_cones1[i].take("LINEALITY_SPACE")<<lineality_space1;
   }
   Array<perl::Object> all_cones2(max_cones2.size());
   for (int i=0; i<max_cones2.size(); ++i) {
      all_cones2[i].create_new(cone_type);
      const Matrix<Coord> p2_cone_vert=rays2.minor(max_cones2[i],All);
      all_cones2[i].take("RAYS")<<p2_cone_vert;
      all_cones2[i].take("LINEALITY_SPACE")<<lineality_space2;
   }

   for (Entire<Array<perl::Object> >::iterator i1=entire(all_cones1); !i1.at_end(); ++i1) {
      for (Entire<Array<perl::Object> >::iterator i2=entire(all_cones2); !i2.at_end(); ++i2) {
         perl::Object inters=CallPolymakeFunction("intersection", *i1, *i2);
         const int inters_dim=inters.give("CONE_DIM");
         if (inters_dim==d || (!complete && inters_dim > 0)) {
            Matrix<Coord> inters_rays=inters.give("RAYS");
            project_to_orthogonal_complement(inters_rays, lineality_space);
            Array<int> ray_indices(inters_rays.rows());
            int index=0;
            for (typename Entire<Rows<Matrix<Coord> > >::const_iterator i=entire(rows(inters_rays)); !i.at_end(); ++i,++index) {
         
               const Vector<Coord> ray=*i;
               const typename hash_map<Vector<Coord>,int>::iterator rep=ray_map.find(ray);
               if (rep!=ray_map.end()) {
                  ray_indices[index]=rep->second;
               } else {
                  ray_indices[index]=rays.rows();
                  ray_map[ray]=rays.rows();
                  rays/=ray;
               }
            }
            Set<int> new_cone;
            for (Entire<sequence>::const_iterator j=entire(sequence(0,index)); !j.at_end(); ++j)
               if (ray_indices[*j]>=0) new_cone.insert(ray_indices[*j]);
            new_max_cones.push_back(new_cone);
         }
      }
   }

   perl::Object f_out("PolyhedralFan");
   if (complete)
      f_out.take("FAN_DIM")<<d;
   f_out.take("COMPLETE") << complete;
   f_out.take("LINEALITY_DIM") << lineality_space.rows();
   f_out.take(complete ? "RAYS" : "INPUT_RAYS")<<rays;
   f_out.take(complete ? "MAXIMAL_CONES" : "INPUT_CONES")<<new_max_cones;
   f_out.take(complete ? "LINEALITY_SPACE" : "INPUT_LINEALITY")<<lineality_space;
   return f_out;
}

UserFunctionTemplate4perl("# Computes the common refinement of two complete fans."
                          "# @param PolyhedralFan f1"
                          "# @param PolyhedralFan f2"
                          "# @return PolyhedralFan",
                          "common_refinement<Coord>(PolyhedralFan<Coord>,PolyhedralFan<Coord>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End: