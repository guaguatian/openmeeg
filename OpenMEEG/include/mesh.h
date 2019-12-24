/*
Project Name : OpenMEEG

© INRIA and ENPC (contributors: Geoffray ADDE, Maureen CLERC, Alexandre
GRAMFORT, Renaud KERIVEN, Jan KYBIC, Perrine LANDREAU, Théodore PAPADOPOULO,
Emmanuel OLIVI
Maureen.Clerc.AT.inria.fr, keriven.AT.certis.enpc.fr,
kybic.AT.fel.cvut.cz, papadop.AT.inria.fr)

The OpenMEEG software is a C++ package for solving the forward/inverse
problems of electroencephalography and magnetoencephalography.

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's authors,  the holders of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/

#pragma once

// for IO:s
#include <iostream>
#include <fstream>

#include <vector>
#include <set>
#include <map>
#include <stack>
#include <string>

#include <om_common.h>
#include <triangle.h>
#include <IOUtils.H>
#include <om_utils.h>
#include <sparse_matrix.h>

#ifdef USE_VTK
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkDataReader.h>
#include <vtkCellArray.h>
#include <vtkCharArray.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#endif

#ifdef USE_GIFTI
extern "C" {
    #include <gifti_io.h>
}
#endif

namespace OpenMEEG {

    enum Filetype { VTK, TRI, BND, MESH, OFF, GIFTI };

    /**
        Mesh class
        \brief Mesh is a collection of triangles
    */

    class OPENMEEG_EXPORT Mesh {
    public:

        typedef std::vector<Triangle*>                VectPTriangle;
        typedef std::vector<Vertex*>                  VectPVertex;
        typedef std::map<const Vertex*,VectPTriangle> AdjacencyMap;

        /// Default constructor

        Mesh(): all_vertices_(0),triangles_() { }

        /// Constructor from scratch (add vertices/triangles one by one)
        /// \param nv space to allocate for vertices
        /// \param nt space to allocate for triangles

        Mesh(const unsigned& nv,const unsigned& nt): allocated(true) {
            all_vertices_ = new Vertices;
            all_vertices_->reserve(nv); // allocates space for the vertices
            triangles_.reserve(nt);
        }

        /// Constructor from another mesh \param m

        Mesh(const Mesh& m) { *this = m; }

        /// Constructor using an existing set of vertices.
        /// \param av vertices
        /// \param name mesh name

        Mesh(Vertices& av,const std::string name=""): mesh_name(name),all_vertices_(&av) {
            set_vertices_.insert(all_vertices_->begin(),all_vertices_->end());
        }

        /// Constructor loading directly a mesh file named \param filename .
        /// Be verbose if \param verbose is true.
        /// The mesh name is \param name .

        Mesh(const std::string& filename,const bool verbose=true,const std::string& name=""):
            mesh_name(name),allocated(true)
        {
            unsigned nb_v = load(filename,false,false);
            all_vertices_ = new Vertices(nb_v); // allocates space for the vertices
            load(filename,verbose);
        }

        /// Destructor

        ~Mesh() { destroy(); }

        std::string&       name()       { return mesh_name; } ///< \return the mesh name
        const std::string& name() const { return mesh_name; } ///< \return the mesh name

        void setName(const std::string& name) { mesh_name = name ; return ;  } ///< setter for the mesh name

        const VectPVertex& vertices()     const { return vertices_;      } ///< \return the vector of pointers to the mesh vertices
              Vertices     all_vertices() const { return *all_vertices_; }

              Triangles& triangles()       { return triangles_; } ///< \return the triangles of the mesh
        const Triangles& triangles() const { return triangles_; } ///< \return the triangles of the mesh

        bool  current_barrier() const { return current_barrier_; }
        bool& current_barrier()       { return current_barrier_; }
        bool  isolated()        const { return isolated_;        }
        bool& isolated()              { return isolated_;        }

        void add_vertex(const Vertex& v); ///< \brief Add vertex to the mesh.

        bool operator==(const Mesh& m) const { return triangles()==m.triangles(); }
        bool operator!=(const Mesh& m) const { return triangles()!=m.triangles(); }

        /// \brief Print info
        ///  Print to std::cout some info about the mesh
        ///  \return void \sa */

        void info(const bool verbose=false) const; ///< \brief Print mesh information.
        bool has_self_intersection() const; ///< \brief Check whether the mesh self-intersects.
        bool intersection(const Mesh&) const; ///< \brief Check whether the mesh intersects another mesh.
        bool has_correct_orientation() const; ///< \brief Check local orientation of mesh triangles.
        void build_mesh_vertices(); ///< \brief Construct mesh vertices from its triangles,
        void generate_indices(); ///< \brief Generate indices (if allocate).
        void update(); ///< \brief Recompute triangles normals, area, and links.
        void merge(const Mesh&,const Mesh&); ///< Merge two meshes.

        /// \brief Get the triangles adjacent to vertex \param V .

        VectPTriangle adjacent_triangles(const Vertex& V) const { return links_.at(&V); }

        /// \brief Get the triangles adjacent to \param triangle .

        VectPTriangle adjacent_triangles(const Triangle& triangle) const {
            std::map<Triangle*,unsigned> mapt;
            VectPTriangle result;
            for (auto& vertex : triangle)
                for (const auto& t2 : adjacent_triangles(*vertex))
                    if (++mapt[t2]==2)
                        result.push_back(t2);
            return result;
        }

        /// Change mesh orientation.

        void change_orientation() {
            for (auto& triangle : triangles())
                triangle.change_orientation();
        }

        void correct_local_orientation(); ///< \brief Correct the local orientation of the mesh triangles.
        void correct_global_orientation(); ///< \brief Correct the global orientation (if there is one).
        double solid_angle(const Vect3& p) const; ///< Given a point p, computes the solid angle of the mesh seen from \param p .
        const VectPTriangle& get_triangles_for_vertex(const Vertex& V) const; ///< \brief Get the triangles associated with vertex V \return the links
        Normal normal(const Vertex& v) const; ///< \brief Get normal at vertex.`
        void laplacian(SymMatrix &A) const; ///< \brief Compute mesh laplacian.

        bool& outermost()       { return outermost_; } /// \brief Returns True if it is an outermost mesh.
        bool  outermost() const { return outermost_; }

        /// \brief Smooth Mesh
        /// \param smoothing_intensity
        /// \param niter
        /// \return void

        void smooth(const double& smoothing_intensity, const unsigned& niter);

        /// \brief Compute the square norm of the surfacic gradient

        void gradient_norm2(SymMatrix &A) const;

        // for IO:s --------------------------------------------------------------------
        /// Read mesh from file
        /// \param filename can be .vtk, .tri (ascii), .off .bnd or .mesh.
        /// Be verbose if \param verbose is true.
        /// Id \param read_all is false then it only returns the total number of vertices.

        unsigned load(const std::string& filename,const bool& verbose=true,const bool& read_all=true);
        unsigned load_tri(std::istream& , const bool& read_all = true);
        unsigned load_tri(const std::string&, const bool& read_all = true);
        unsigned load_bnd(std::istream& , const bool& read_all = true);
        unsigned load_bnd(const std::string&, const bool& read_all = true);
        unsigned load_off(std::istream& , const bool& read_all = true);
        unsigned load_off(const std::string&, const bool& read_all = true);
        unsigned load_mesh(std::istream& , const bool& read_all = true);
        unsigned load_mesh(const std::string&, const bool& read_all = true);

    #ifdef USE_VTK
        unsigned load_vtk(std::istream& , const bool& read_all = true);
        unsigned load_vtk(const std::string&, const bool& read_all = true);
        unsigned get_data_from_vtk_reader(vtkPolyDataReader* vtkMesh, const bool& read_all);
    #else
        template <typename T>
        unsigned load_vtk(T, const bool& read_all = true) {
            std::cerr << "You have to compile OpenMEEG with VTK to read VTK/VTP files. (specify USE_VTK to cmake)" << std::endl;
            exit(1);
        }
    #endif

    #ifdef USE_GIFTI
        unsigned load_gifti(const std::string&, const bool& read_all = true);
        void save_gifti(const std::string&) const;
    #else
        template <typename T>
        unsigned load_gifti(T, const bool&) {
            std::cerr << "You have to compile OpenMEEG with GIFTI to read GIFTI files" << std::endl;
            exit(1);
        }
        template <typename T>
        void save_gifti(T) const {
            std::cerr << "You have to compile OpenMEEG with GIFTI to read GIFTI files" << std::endl;
            exit(1);
        }
    #endif

        /// Save mesh to file
        /// \param filename can be .vtk, .tri (ascii), .bnd, .off or .mesh */

        void save(const std::string& filename) const ;
        void save_vtk(const std::string&)  const;
        void save_bnd(const std::string&)  const;
        void save_tri(const std::string&)  const;
        void save_off(const std::string&)  const;
        void save_mesh(const std::string&) const;

        // IO:s

        Mesh& operator=(const Mesh& m) {
            if (this!=&m)
                copy(m);
            return *this;
        }

    private:

        /// map the edges with an unsigned

        typedef std::map<std::pair<const Vertex *, const Vertex *>, int> EdgeMap;

        void destroy();
        void copy(const Mesh&);

        // regarding mesh orientation

        const EdgeMap compute_edge_map() const;
        bool  triangle_intersection(const Triangle&,const Triangle&) const;

        /// P1gradient: aux function to compute the surfacic gradient

        Vect3 P1gradient(const Vect3& p0,const Vect3& p1,const Vect3& p2) const { return crossprod(p1,p2)/det(p0,p1,p2); }

        /// P0gradient_norm2: aux function to compute the square norm of the surfacic gradient

        double P0gradient_norm2(const Triangle& t1,const Triangle& t2) const {
            return sqr(dotprod(t1.normal(),t2.normal()))/(t1.center()-t2.center()).norm2();
        }

        // Create the map that for each vertex gives the triangles containing it.

        void make_adjacencies() {
            links_.clear();
            for (auto& triangle : triangles())
                for (const auto& vertex : triangle)
                    links_[vertex].push_back(&triangle);
        }

        std::string      mesh_name = "";     ///< Name of the mesh.
        AdjacencyMap     links_;             ///< links[&v] are the triangles that contain vertex v.
        Vertices*        all_vertices_;      ///< Pointer to all the vertices.
        VectPVertex      vertices_;          ///< Vector of pointers to the mesh vertices.
        Triangles        triangles_;         ///< Vector of triangles.
        bool             outermost_ = false; ///< Is it an outermost mesh ? (i.e does it touch the Air domain)
        bool             allocated = false;  ///< Are the vertices allocate within the mesh or shared ?
        std::set<Vertex> set_vertices_;

        /// Multiple 0 conductivity domains

        bool             current_barrier_ = false;
        bool             isolated_        = false;
    };

    /// A vector of Mesh is called Meshes

    typedef std::vector<Mesh> Meshes;
}
