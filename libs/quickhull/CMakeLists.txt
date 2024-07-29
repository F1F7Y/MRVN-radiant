add_library(quickhull STATIC
            Structs/Mesh.hpp
            Structs/Plane.hpp
            Structs/Pool.hpp
            Structs/Ray.hpp
            Structs/Vector3.hpp
            Structs/VertexDataSource.hpp
            ConvexHull.hpp
            HalfEdgeMesh.hpp
            MathUtils.hpp
            Tests/QuickHullTests.cpp
            Tests/main.cpp
            QuickHull.cpp
            QuickHull.hpp
           )
