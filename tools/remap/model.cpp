/* -------------------------------------------------------------------------------

   Copyright (C) 1999-2007 id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

   ----------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* dependencies */
#include "remap.h"

#include "model.h"

#include "assimp/Importer.hpp"
#include "assimp/importerdesc.h"
#include "assimp/Logger.hpp"
#include "assimp/DefaultLogger.hpp"
#include "assimp/IOSystem.hpp"
#include "assimp/MemoryIOWrapper.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/mesh.h"

#include <map>


class AssLogger : public Assimp::Logger
{
public:
	void OnDebug( const char* message ) override {
#ifdef _DEBUG
		Sys_Printf( "%s\n", message );
#endif
	}
	void OnVerboseDebug( const char *message ) override {
#ifdef _DEBUG
		Sys_FPrintf( SYS_VRB, "%s\n", message );
#endif
	}
	void OnInfo( const char* message ) override {
#ifdef _DEBUG
		Sys_Printf( "%s\n", message );
#endif
	}
	void OnWarn( const char* message ) override {
		Sys_Warning( "%s\n", message );
	}
	void OnError( const char* message ) override {
		Sys_FPrintf( SYS_WRN, "ERROR: %s\n", message ); /* let it be a warning, since radiant stops monitoring on error message flag */
	}

	bool attachStream( Assimp::LogStream *pStream, unsigned int severity ) override {
		return false;
	}
	bool detachStream( Assimp::LogStream *pStream, unsigned int severity ) override {
		return false;
	}
};


class AssIOSystem : public Assimp::IOSystem
{
public:
	// -------------------------------------------------------------------
	/** @brief Tests for the existence of a file at the given path.
	 *
	 * @param pFile Path to the file
	 * @return true if there is a file with this path, else false.
	 */
	bool Exists( const char* pFile ) const override {
		return vfsFileExists( pFile );
	}

	// -------------------------------------------------------------------
	/** @brief Returns the system specific directory separator
	 *  @return System specific directory separator
	 */
	char getOsSeparator() const override {
		return '/';
	}

	// -------------------------------------------------------------------
	/** @brief Open a new file with a given path.
	 *
	 *  When the access to the file is finished, call Close() to release
	 *  all associated resources (or the virtual dtor of the IOStream).
	 *
	 *  @param pFile Path to the file
	 *  @param pMode Desired file I/O mode. Required are: "wb", "w", "wt",
	 *         "rb", "r", "rt".
	 *
	 *  @return New IOStream interface allowing the lib to access
	 *         the underlying file.
	 *  @note When implementing this class to provide custom IO handling,
	 *  you probably have to supply an own implementation of IOStream as well.
	 */
	Assimp::IOStream* Open( const char* pFile, const char* pMode = "rb" ) override {
		if ( CMemBuffer boo = vfsLoadFile( pFile ) ) {
			return new Assimp::MemoryIOStream( boo.release(), boo.size(), true );
		}
		return nullptr;
	}

	// -------------------------------------------------------------------
	/** @brief Closes the given file and releases all resources
	 *    associated with it.
	 *  @param pFile The file instance previously created by Open().
	 */
	void Close( Assimp::IOStream* pFile ) override {
		delete pFile;
	}

	// -------------------------------------------------------------------
	/** @brief CReates an new directory at the given path.
	 *  @param  path    [in] The path to create.
	 *  @return True, when a directory was created. False if the directory
	 *           cannot be created.
	 */
	bool CreateDirectory( const std::string &path ) override {
		Error( "AssIOSystem::CreateDirectory" );
		return false;
	}

	// -------------------------------------------------------------------
	/** @brief Will change the current directory to the given path.
	 *  @param path     [in] The path to change to.
	 *  @return True, when the directory has changed successfully.
	 */
	bool ChangeDirectory( const std::string &path ) override {
		Error( "AssIOSystem::ChangeDirectory" );
		return false;
	}

	bool DeleteFile( const std::string &file ) override {
		Error( "AssIOSystem::DeleteFile" );
		return false;
	}

private:
};

static Assimp::Importer *s_assImporter = nullptr;

void assimp_init(){
	s_assImporter = new Assimp::Importer();

	s_assImporter->SetPropertyBool( AI_CONFIG_PP_PTV_ADD_ROOT_TRANSFORMATION, true );
	s_assImporter->SetPropertyInteger( AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE );
	s_assImporter->SetPropertyString( AI_CONFIG_IMPORT_MDL_COLORMAP, "gfx/palette.lmp" ); // Q1 palette, default is fine too
	s_assImporter->SetPropertyBool( AI_CONFIG_IMPORT_MD3_LOAD_SHADERS, false );
	s_assImporter->SetPropertyString( AI_CONFIG_IMPORT_MD3_SHADER_SRC, "scripts/" );
	s_assImporter->SetPropertyBool( AI_CONFIG_IMPORT_MD3_HANDLE_MULTIPART, false );
	s_assImporter->SetPropertyInteger( AI_CONFIG_PP_RVC_FLAGS, aiComponent_TANGENTS_AND_BITANGENTS ); // varying tangents prevent aiProcess_JoinIdenticalVertices

	Assimp::DefaultLogger::set( new AssLogger );

	s_assImporter->SetIOHandler( new AssIOSystem );
}

struct ModelNameFrame
{
	CopiedString m_name;
	int m_frame;
	bool operator<( const ModelNameFrame& other ) const {
		const int cmp = string_compare_nocase( m_name.c_str(), other.m_name.c_str() );
		return cmp != 0? cmp < 0 : m_frame < other.m_frame;
	}
};
struct AssModel
{
	struct AssModelMesh : public AssMeshWalker
	{
		const aiMesh *m_mesh;
		CopiedString m_shader;

		AssModelMesh( const aiScene *scene, const aiMesh *mesh, const char *rootPath ) : m_mesh( mesh ){
			aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

			aiString matname = material->GetName();
#ifdef _DEBUG
						Sys_Printf( "matname: %s\n", matname.C_Str() );
#endif
			if( aiString texname;
			    aiReturn_SUCCESS == material->Get( AI_MATKEY_TEXTURE_DIFFUSE(0), texname )
			 && texname.length != 0
			 && !string_equal_prefix_nocase( matname.C_Str(), "textures/" ) /* matname looks intentionally named as ingame shader */
			 && !string_equal_prefix_nocase( matname.C_Str(), "textures\\" )
			 && !string_equal_prefix_nocase( matname.C_Str(), "models/" )
			 && !string_equal_prefix_nocase( matname.C_Str(), "models\\" ) ){
#ifdef _DEBUG
							Sys_Printf( "texname: %s\n", texname.C_Str() );
#endif
				m_shader = StringOutputStream()( PathCleaned( PathExtensionless( texname.C_Str() ) ) );

			}
			else{
				m_shader = StringOutputStream()( PathCleaned( PathExtensionless( matname.C_Str() ) ) );
			}

			const CopiedString oldShader( m_shader );
			if( strchr( m_shader.c_str(), '/' ) == nullptr ){ /* texture is likely in the folder, where model is */
				m_shader = StringOutputStream()( rootPath, m_shader );
			}
			else{
				const char *name = m_shader.c_str();
				if( name[0] == '/' || ( name[0] != '\0' && name[1] == ':' ) || strstr( name, ".." ) ){ /* absolute path or with .. */
					const char* p;
					if( ( p = string_in_string_nocase( name, "/models/" ) )
					 || ( p = string_in_string_nocase( name, "/textures/" ) ) ){
						m_shader = p + 1;
					}
					else{
						m_shader = StringOutputStream()( rootPath, path_get_filename_start( name ) );
					}
				}
			}

			if( oldShader != m_shader )
				Sys_FPrintf( SYS_VRB, "substituting: %s -> %s\n", oldShader.c_str(), m_shader.c_str() );
		}

		void forEachFace( std::function<void( const Vector3 ( &xyz )[3], const Vector2 ( &st )[3])> visitor ) const override {
			for ( const aiFace& face : Span( m_mesh->mFaces, m_mesh->mNumFaces ) ){
				// if( face.mNumIndices == 3 )
				Vector3 xyz[3];
				Vector2 st[3];
				for( size_t n = 0; n < 3; ++n ){
					const auto i = face.mIndices[n];
					xyz[n] = { m_mesh->mVertices[i].x, m_mesh->mVertices[i].y, m_mesh->mVertices[i].z };
					if( m_mesh->HasTextureCoords( 0 ) )
						st[n] = { m_mesh->mTextureCoords[0][i].x, m_mesh->mTextureCoords[0][i].y };
					else
						st[n] = { 0, 0 };
				}
				visitor( xyz, st );
			}
		}
		const char *getShaderName() const override {
			return m_shader.c_str();
		}
	};

	aiScene *m_scene;
	std::vector<AssModelMesh> m_meshes;

	AssModel( aiScene *scene, const char *modelname ) : m_scene( scene ){
		m_meshes.reserve( scene->mNumMeshes );
		const auto rootPath = StringOutputStream()( PathCleaned( PathFilenameless( modelname ) ) );
		const auto traverse = [&]( const auto& self, const aiNode* node ) -> void {
			for( size_t n = 0; n < node->mNumMeshes; ++n ){
				const aiMesh *mesh = scene->mMeshes[node->mMeshes[n]];
				if( mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE ){
					m_meshes.emplace_back( scene, mesh, rootPath );
				}
			}

			// traverse all children
			for ( size_t n = 0; n < node->mNumChildren; ++n ){
				self( self, node->mChildren[n] );
			}
		};

		traverse( traverse, scene->mRootNode );
	}
};

static std::map<ModelNameFrame, AssModel> s_assModels;



/*
   LoadModel() - ydnar
   loads a picoModel and returns a pointer to the picoModel_t struct or NULL if not found
 */

static AssModel *LoadModel( const char *name, int frame ){
	/* dummy check */
	if ( strEmptyOrNull( name ) ) {
		return nullptr;
	}

	/* try to find existing picoModel */
	auto it = s_assModels.find( ModelNameFrame{ name, frame } );
	if( it != s_assModels.end() ){
		return &it->second;
	}

	unsigned flags = //aiProcessPreset_TargetRealtime_Fast
	            //    | aiProcess_FixInfacingNormals
	                 aiProcess_GenNormals
	               | aiProcess_JoinIdenticalVertices
	               | aiProcess_Triangulate
	               | aiProcess_GenUVCoords
	               | aiProcess_SortByPType
	               | aiProcess_FindDegenerates
	               | aiProcess_FindInvalidData
	               | aiProcess_ValidateDataStructure
	               | aiProcess_FlipUVs
	               | aiProcess_FlipWindingOrder
	               | aiProcess_PreTransformVertices
	               | aiProcess_RemoveComponent
	               | aiProcess_SplitLargeMeshes;
	// rotate the whole scene 90 degrees around the x axis to convert assimp's Y = UP to Quakes's Z = UP
	s_assImporter->SetPropertyMatrix( AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION, aiMatrix4x4( 1, 0, 0, 0,
	                                                                                     0, 0, -1, 0,
	                                                                                     0, 1, 0, 0,
	                                                                                     0, 0, 0, 1 ) ); // aiMatrix4x4::RotationX( c_half_pi )

	s_assImporter->SetPropertyInteger( AI_CONFIG_PP_SLM_VERTEX_LIMIT, maxSurfaceVerts ); // TODO this optimal and with respect to lightmapped/not
	s_assImporter->SetPropertyInteger( AI_CONFIG_IMPORT_GLOBAL_KEYFRAME, frame );

	const aiScene *scene = s_assImporter->ReadFile( name, flags );

	if( scene != nullptr ){
		if( scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE )
			Sys_Warning( "AI_SCENE_FLAGS_INCOMPLETE\n" );
		return &s_assModels.emplace( ModelNameFrame{ name, frame }, AssModel( s_assImporter->GetOrphanedScene(), name ) ).first->second;
	}
	else{
		return nullptr; // TODO /* if loading failed, make a bogus model to silence the rest of the warnings */
	}
}

std::vector<const AssMeshWalker*> LoadModelWalker( const char *name, int frame ){
	AssModel *model = LoadModel( name, frame );
	std::vector<const AssMeshWalker*> vector;
	if( model != nullptr )
		std::for_each( model->m_meshes.begin(), model->m_meshes.end(), [&vector]( const auto& val ){
			vector.push_back( &val );
		} );
	return vector;
}



enum EModelFlags{
	eRMG_BSP = 1 << 0,
	eClipModel = 1 << 1,
	eForceMeta = 1 << 2,
	eExtrudeFaceNormals = 1 << 3,
	eExtrudeTerrain = 1 << 4,
	eColorToAlpha = 1 << 5,
	eNoSmooth = 1 << 6,
	eExtrudeVertexNormals = 1 << 7,
	ePyramidaClip = 1 << 8,
	eExtrudeDownwards = 1 << 9,
	eExtrudeUpwards = 1 << 10,
	eMaxExtrude = 1 << 11,
	eAxialBackplane = 1 << 12,
	eClipFlags = eClipModel | eExtrudeFaceNormals | eExtrudeTerrain | eExtrudeVertexNormals | ePyramidaClip | eExtrudeDownwards | eExtrudeUpwards | eMaxExtrude | eAxialBackplane,
};

inline void nonax_clip_dbg( const Plane3f (&p)[3] ){
#if 0
	for ( int j = 0; j < 3; ++j ){
		for ( int k = 0; k < 3; ++k ){
			const Vector3& n = p[j].normal();
			if ( fabs( n[k] ) < 0.00025 && n[k] != 0 ){
				Sys_Printf( "nonax nrm %6.17f %6.17f %6.17f\n", n[0], n[1], n[2] );
			}
		}
	}
#endif
}

inline size_t normal_make_axial( Vector3& normal ){
	const size_t i = vector3_max_abs_component_index( normal );
	normal = normal[i] >= 0? g_vector3_axes[i] : -g_vector3_axes[i];
	return i;
}

template<size_t N> // N = 4 or 5
static void make_brush_sides( const Plane3f plane, const Plane3f (&p)[3], const Plane3f& reverse, Vector3 (&points)[4], shaderInfo_t *si ){
	/* set up brush sides */
	buildBrush.sides.clear(); // clear, so resize() will value-initialize elements
	buildBrush.sides.resize( N );

	if( debugClip ){
		buildBrush.sides[ 0 ].shaderInfo = ShaderInfoForShader( "debugclip2" );
		for ( size_t i = 1; i < N; ++i )
			buildBrush.sides[i].shaderInfo = ShaderInfoForShader( "debugclip" );
	}
	else{
		buildBrush.sides[0].shaderInfo = si;
		buildBrush.sides[0].surfaceFlags = si->surfaceFlags;
		for ( size_t i = 1; i < N; ++i )
			buildBrush.sides[i].shaderInfo = NULL;  // don't emit these faces as draw surfaces, should make smaller BSPs; hope this works
	}

	points[3] = points[0]; // for cyclic usage

	buildBrush.sides[0].planenum = FindFloatPlane( plane, 3, points );
	buildBrush.sides[1].planenum = FindFloatPlane( p[0], 2, &points[0] ); // p[0] contains points[0] and points[1]
	buildBrush.sides[2].planenum = FindFloatPlane( p[1], 2, &points[1] ); // p[1] contains points[1] and points[2]
	buildBrush.sides[3].planenum = FindFloatPlane( p[2], 2, &points[2] ); // p[2] contains points[2] and points[0] (copied to points[3])
	if constexpr( N == 5 )
		buildBrush.sides[4].planenum = FindFloatPlane( reverse, 0, NULL );
}

static void ClipModel( int spawnFlags, float clipDepth, shaderInfo_t *si, const mapDrawSurface_t *ds, const char *modelName, entity_t& entity ){
	const int spf = ( spawnFlags & ( eClipFlags & ~eClipModel ) );

	/* ydnar: giant hack land: generate clipping brushes for model triangles */
	if ( ( si->clipModel && spf == 0 ) // default CLIPMODEL
	  || ( spawnFlags & eClipFlags ) == eClipModel //default CLIPMODEL
	  || spf == eExtrudeFaceNormals
	  || spf == eExtrudeTerrain
	  || spf == eExtrudeVertexNormals
	  || spf == ePyramidaClip
	  || spf == eExtrudeDownwards
	  || spf == eExtrudeUpwards
	  || spf == eAxialBackplane // default sides + axial backplane
	  || spf == ( eExtrudeFaceNormals | ePyramidaClip ) // extrude 45
	  || spf == ( eExtrudeTerrain | eMaxExtrude )
	  || spf == ( eExtrudeTerrain | eAxialBackplane )
	  || spf == ( eExtrudeVertexNormals | ePyramidaClip ) // vertex normals + don't check for sides, sticking outwards
	  || spf == ( ePyramidaClip | eAxialBackplane ) // pyramid with 3 of 4 sides axial (->small bsp)
	  || spf == ( eExtrudeDownwards | eExtrudeUpwards )
	  || spf == ( eExtrudeDownwards | eMaxExtrude )
	  || spf == ( eExtrudeDownwards | eAxialBackplane )
	  || spf == ( eExtrudeDownwards | eExtrudeUpwards | eMaxExtrude )
	  || spf == ( eExtrudeDownwards | eExtrudeUpwards | eAxialBackplane )
	  || spf == ( eExtrudeUpwards | eMaxExtrude )
	  || spf == ( eExtrudeUpwards | eAxialBackplane ) ){
		int i, j, k;
		//int ok=0, notok=0;
		float limDepth = 0;
		if ( clipDepth < 0 ){
			limDepth = -clipDepth;
			clipDepth = 2.0;
		}
		Vector3 points[ 4 ];
		Plane3f plane, reverse, p[3];
		MinMax minmax;
		Vector3 avgDirection( 0 );
		int axis;

		/* temp hack */
		if ( !si->clipModel && !( si->compileFlags & C_SOLID ) ) {
			return;
		}

		//wont snap these in normal way, or will explode
		// const double normalEpsilon_save = normalEpsilon;
		//normalEpsilon = 0.000001;


		if ( ( spf & eMaxExtrude ) || ( spf & eExtrudeTerrain ) ){

			for ( i = 0; i < ds->numIndexes; i += 3 ){
				for ( j = 0; j < 3; ++j ){
					points[j] = ds->verts[ds->indexes[i + j]].xyz;
				}
				if ( PlaneFromPoints( plane, points ) ){
					if ( spf & eExtrudeTerrain )
						avgDirection += plane.normal();	//calculate average mesh facing direction

					//get min/max
					for ( j = 0; j < 3; ++j ){
						minmax.extend( points[j] );
					}
				}
			}
			//unify avg direction
			if ( spf & eExtrudeTerrain ){
				if ( vector3_length( avgDirection ) == 0 )
					avgDirection = g_vector3_axis_z;
				axis = normal_make_axial( avgDirection );
			}
		}

		/* prepare a brush */
		buildBrush.sides.reserve( MAX_BUILD_SIDES );
		buildBrush.entityNum = entity.mapEntityNum;
		buildBrush.contentShader = si;
		buildBrush.compileFlags = si->compileFlags;
		buildBrush.contentFlags = si->contentFlags;
		buildBrush.detail = true;

		/* walk triangle list */
		for ( i = 0; i < ds->numIndexes; i += 3 ){
			/* make points */
			for ( j = 0; j < 3; ++j ){
				/* copy xyz */
				points[j] = ds->verts[ds->indexes[i + j] ].xyz;
			}

			/* make plane for triangle */
			if ( PlaneFromPoints( plane, points ) ) {
				//snap points before using them for further calculations
				//precision suffers a lot, when two of normal values are under .00025 (often no collision, knocking up effect in ioq3)
				//also broken drawsurfs in case of normal brushes
				bool snpd = false;
				for ( j = 0; j < 3; ++j )
				{
					if ( fabs( plane.normal()[j] ) < 0.00025 && fabs( plane.normal()[( j + 1) % 3] ) < 0.00025
					&& ( plane.normal()[j] != 0.0 || plane.normal()[( j + 1 ) % 3] != 0.0 ) ){
						const Vector3 cnt = ( points[0] + points[1] + points[2] ) / 3.0;
						points[0][( j + 2 ) % 3] = points[1][(j + 2 ) % 3] = points[2][( j + 2 ) % 3] = cnt[( j + 2 ) % 3];
						snpd = true;
						break;
					}
				}

				//snap pairs of points to prevent bad side planes
				for ( j = 0; j < 3; ++j )
				{
					const Vector3 nrm = VectorNormalized( points[j] - points[( j + 1 ) % 3] );
					for ( k = 0; k < 3; ++k )
					{
						if ( nrm[k] != 0.0 && fabs( nrm[k] ) < 0.00025 ){
							//Sys_Printf( "b4(%6.6f %6.6f %6.6f)(%6.6f %6.6f %6.6f)\n", points[j][0], points[j][1], points[j][2], points[(j+1)%3][0], points[(j+1)%3][1], points[(j+1)%3][2] );
							points[j][k] = points[( j + 1 ) % 3][k] = ( points[j][k] + points[( j + 1 ) % 3][k] ) / 2.0;
							//Sys_Printf( "sn(%6.6f %6.6f %6.6f)(%6.6f %6.6f %6.6f)\n", points[j][0], points[j][1], points[j][2], points[(j+1)%3][0], points[(j+1)%3][1], points[(j+1)%3][2] );
							snpd = true;
						}
					}
				}

				if ( snpd ) {
					PlaneFromPoints( plane, points );
					snpd = false;
				}

				//vector-is-close-to-be-on-axis check again, happens after previous code sometimes
				for ( j = 0; j < 3; ++j )
				{
					if ( fabs( plane.normal()[j] ) < 0.00025 && fabs( plane.normal()[( j + 1 ) % 3] ) < 0.00025
					&& ( plane.normal()[j] != 0.0 || plane.normal()[( j + 1 ) % 3] != 0.0 ) ){
						const Vector3 cnt = ( points[0] + points[1] + points[2] ) / 3.0;
						points[0][( j + 2 ) % 3] = points[1][( j + 2 ) % 3] = points[2][( j + 2 ) % 3] = cnt[( j + 2 ) % 3];
						PlaneFromPoints( plane, points );
						break;
					}
				}

				//snap single snappable normal components
				for ( j = 0; j < 3; ++j )
				{
					if ( plane.normal()[j] != 0 && fabs( plane.normal()[j] ) < 0.00005 ){
						plane.normal()[j] = 0;
						snpd = true;
					}
				}

				//adjust plane dist
				if ( snpd ) {
					const Vector3 cnt = ( points[0] + points[1] + points[2] ) / 3.0;
					VectorNormalize( plane.normal() );
					plane.dist() = vector3_dot( plane.normal(), cnt );

					//project points to resulting plane to keep intersections precision
					for ( j = 0; j < 3; ++j )
					{
						//Sys_Printf( "b4 %i (%6.7f %6.7f %6.7f)\n", j, points[j][0], points[j][1], points[j][2] );
						points[j] = plane3_project_point( plane, points[j] );
						//Sys_Printf( "sn %i (%6.7f %6.7f %6.7f)\n", j, points[j][0], points[j][1], points[j][2] );
					}
					//Sys_Printf( "sn pln (%6.7f %6.7f %6.7f %6.7f)\n", plane.a, plane.b, plane.c, plane.d );
					//PlaneFromPoints( plane, points );
					//Sys_Printf( "pts pln (%6.7f %6.7f %6.7f %6.7f)\n", plane.a, plane.b, plane.c, plane.d );
				}

				/* sanity check */
				{
					const Vector3 d1 = points[1] - points[0];
					const Vector3 d2 = points[2] - points[0];
					const Vector3 normal = vector3_cross( d2, d1 );
					/* https://en.wikipedia.org/wiki/Cross_product#Geometric_meaning
					   cross( a, b ).length = a.length b.length sin( angle ) */
					const double lengthsSquared = vector3_length_squared( d1 ) * vector3_length_squared( d2 );
					if ( lengthsSquared == 0 || ( vector3_length_squared( normal ) / lengthsSquared ) < 1e-8 ) {
						Sys_Warning( "triangle (%6.0f %6.0f %6.0f) (%6.0f %6.0f %6.0f) (%6.0f %6.0f %6.0f) of %s was not autoclipped: points on line\n",
						             points[0][0], points[0][1], points[0][2], points[1][0], points[1][1], points[1][2], points[2][0], points[2][1], points[2][2], modelName );
						continue;
					}
				}


				if ( spf == ( ePyramidaClip | eAxialBackplane ) ){ // pyramid with 3 of 4 sides axial (->small bsp)

					for ( j = 0; j < 3; ++j )
						if ( fabs( plane.normal()[j] ) < 0.05 && fabs( plane.normal()[( j + 1 ) % 3] ) < 0.05 ) //no way, close to lay on two axes
							goto default_CLIPMODEL;

					// best axial normal
					Vector3 bestNormal = plane.normal();
					axis = normal_make_axial( bestNormal );

					float mindist = 999999;

					for ( j = 0; j < 3; ++j ){ // planes
						float bestdist = 999999, bestangle = 1;

						for ( k = 0; k < 3; ++k ){ // axes
							Vector3 nrm = points[( j + 1 ) % 3] - points[j];
							if ( k == axis ){
								reverse.normal() = vector3_cross( bestNormal, nrm );
							}
							else{
								Vector3 vnrm( 0 );
								if ( ( k + 1 ) % 3 == axis ){
									if ( nrm[( k + 2 ) % 3] == 0 )
										continue;
									vnrm[( k + 2 ) % 3] = nrm[( k + 2 ) % 3];
								}
								else{
									if ( nrm[( k + 1 ) % 3] == 0 )
										continue;
									vnrm[( k + 1 ) % 3] = nrm[( k + 1 ) % 3];
								}
								const Vector3 enrm = vector3_cross( bestNormal, vnrm );
								reverse.normal() = vector3_cross( enrm, nrm );
							}
							VectorNormalize( reverse.normal() );
							reverse.dist() = vector3_dot( points[ j ], reverse.normal() );
							//check facing, thickness
							const float currdist = reverse.dist() - vector3_dot( reverse.normal(), points[( j + 2 ) % 3] );
							const float currangle = vector3_dot( reverse.normal(), plane.normal() );
							if ( ( ( currdist > 0.1 ) && ( currdist < bestdist ) && ( currangle < 0 ) ) ||
							     ( ( currangle >= 0 ) && ( currangle <= bestangle ) ) ){
								bestangle = currangle;
								if ( currangle < 0 )
									bestdist = currdist;
								p[j] = reverse;
							}
						}
						if ( bestdist == 999999 && bestangle == 1 ){
							// Sys_Printf( "default_CLIPMODEL\n" );
							goto default_CLIPMODEL;
						}
						value_minimize( mindist, bestdist );
					}
					if ( (limDepth != 0.0) && (mindist > limDepth) )
						goto default_CLIPMODEL;

					nonax_clip_dbg( p );

					make_brush_sides<4>( plane, p, reverse, points, si );
				}


				else if ( spf == eExtrudeTerrain
				       || spf == eExtrudeDownwards
				       || spf == eExtrudeUpwards
				       || spf == eAxialBackplane
				       || spf == ( eExtrudeTerrain | eMaxExtrude )
				       || spf == ( eExtrudeTerrain | eAxialBackplane )
				       || spf == ( eExtrudeDownwards | eExtrudeUpwards )
				       || spf == ( eExtrudeDownwards | eMaxExtrude )
				       || spf == ( eExtrudeDownwards | eAxialBackplane )
				       || spf == ( eExtrudeDownwards | eExtrudeUpwards | eMaxExtrude )
				       || spf == ( eExtrudeDownwards | eExtrudeUpwards | eAxialBackplane )
				       || spf == ( eExtrudeUpwards | eMaxExtrude )
				       || spf == ( eExtrudeUpwards | eAxialBackplane ) ){

					Vector3 bestNormal;

					if ( spf & eExtrudeTerrain ){ //autodirection
						bestNormal = avgDirection;
					}
					else{
						axis = 2;
						if ( ( spf & eExtrudeDownwards ) && ( spf & eExtrudeUpwards ) ){
							bestNormal = plane.normal()[2] >= 0? g_vector3_axis_z : -g_vector3_axis_z;
						}
						else if ( spf & eExtrudeDownwards ){
							bestNormal = g_vector3_axis_z;
						}
						else if ( spf & eExtrudeUpwards ){
							bestNormal = -g_vector3_axis_z;
						}
						else{ // best axial normal
							bestNormal = plane.normal();
							axis = normal_make_axial( bestNormal );
						}
					}

					if ( vector3_dot( plane.normal(), bestNormal ) < 0.05 ){
						goto default_CLIPMODEL;
					}


					/* make side planes */
					for ( j = 0; j < 3; ++j )
					{
						p[j].normal() = VectorNormalized( vector3_cross( bestNormal, points[( j + 1 ) % 3] - points[j] ) );
						p[j].dist() = vector3_dot( points[j], p[j].normal() );
					}

					/* make back plane */
					if ( spf & eMaxExtrude ){ //max extrude
						reverse.normal() = -bestNormal;
						if ( bestNormal[axis] > 0 ){
							reverse.dist() = -minmax.mins[axis] + clipDepth;
						}
						else{
							reverse.dist() = minmax.maxs[axis] + clipDepth;
						}
					}
					else if ( spf & eAxialBackplane ){ //axial backplane
						reverse.normal() = -bestNormal;
						reverse.dist() = points[0][axis];
						if ( bestNormal[axis] > 0 ){
							for ( j = 1; j < 3; ++j ){
								value_minimize( reverse.dist(), points[j][axis] );
							}
							reverse.dist() = -reverse.dist() + clipDepth;
						}
						else{
							for ( j = 1; j < 3; ++j ){
								value_maximize( reverse.dist(), points[j][axis] );
							}
							reverse.dist() += clipDepth;
						}
						if ( limDepth != 0.0 ){
							Vector3 cnt = points[0];
							if ( bestNormal[axis] > 0 ){
								for ( j = 1; j < 3; ++j ){
									if ( points[j][axis] > cnt[axis] ){
										cnt = points[j];
									}
								}
							}
							else {
								for ( j = 1; j < 3; ++j ){
									if ( points[j][axis] < cnt[axis] ){
										cnt = points[j];
									}
								}
							}
							cnt = plane3_project_point( reverse, cnt );
							if ( -plane3_distance_to_point( plane, cnt ) > limDepth ){
								reverse = plane3_flipped( plane );
								reverse.dist() += clipDepth;
							}
						}
					}
					else{	//normal backplane
						reverse = plane3_flipped( plane );
						reverse.dist() += clipDepth;
					}

					nonax_clip_dbg( p );

					make_brush_sides<5>( plane, p, reverse, points, si );
				}


				else if ( spf == ( eExtrudeFaceNormals | ePyramidaClip ) ){	// extrude 45
					//45 degrees normals for side planes
					for ( j = 0; j < 3; ++j )
					{
						const Vector3 nrm = points[( j + 1 ) % 3] - points[ j ];
						Vector3 enrm = VectorNormalized( vector3_cross( plane.normal(), nrm ) );
						enrm += plane.normal();
						VectorNormalize( enrm );
						/* make side planes */
						p[j].normal() = VectorNormalized( vector3_cross( enrm, nrm ) );
						p[j].dist() = vector3_dot( points[j], p[j].normal() );
						//snap nearly axial side planes
						snpd = false;
						for ( k = 0; k < 3; ++k )
						{
							if ( fabs( p[j].normal()[k] ) < 0.00025 && p[j].normal()[k] != 0.0 ){
								p[j].normal()[k] = 0.0;
								snpd = true;
							}
						}
						if ( snpd ){
							VectorNormalize( p[j].normal() );
							p[j].dist() = vector3_dot( ( points[j] + points[( j + 1 ) % 3] ) / 2.0, p[j].normal() );
						}
					}

					/* make back plane */
					reverse = plane3_flipped( plane );
					reverse.dist() += clipDepth;

					make_brush_sides<5>( plane, p, reverse, points, si );
				}


				else if ( spf == eExtrudeVertexNormals
				       || spf == ( eExtrudeVertexNormals | ePyramidaClip ) ){ // vertex normals + don't check for sides, sticking outwards
					Vector3 Vnorm[3], Enorm[3];
					/* get vertex normals */
					for ( j = 0; j < 3; ++j ){
						/* copy normal */
						Vnorm[j] = ds->verts[ds->indexes[i + j]].normal;
					}

					//avg normals for side planes
					for ( j = 0; j < 3; ++j )
					{
						Enorm[ j ] = VectorNormalized( Vnorm[ j ] + Vnorm[( j + 1 ) % 3] );
						//check fuer bad ones
						const Vector3 nrm = VectorNormalized( vector3_cross( plane.normal(), points[( j + 1 ) % 3] - points[ j ] ) );
						//check for negative or outside direction
						if ( vector3_dot( Enorm[j], plane.normal() ) > 0.1 ){
							if ( ( vector3_dot( Enorm[j], nrm ) > -0.2 ) || ( spf & ePyramidaClip ) ){
								//ok++;
								continue;
							}
						}
						//notok++;
						//Sys_Printf( "faulty Enormal %i/%i\n", notok, ok );
						//use 45 normal
						Enorm[ j ] = plane.normal() + nrm;
						VectorNormalize( Enorm[ j ] );
					}

					/* make side planes */
					for ( j = 0; j < 3; ++j )
					{
						p[j].normal() = VectorNormalized( vector3_cross( Enorm[j], points[( j + 1 ) % 3] - points[j] ) );
						p[j].dist() = vector3_dot( points[j], p[j].normal() );
						//snap nearly axial side planes
						snpd = false;
						for ( k = 0; k < 3; ++k )
						{
							if ( fabs( p[j].normal()[k] ) < 0.00025 && p[j].normal()[k] != 0.0 ){
								//Sys_Printf( "init plane %6.8f %6.8f %6.8f %6.8f\n", p[j].a, p[j].b, p[j].c, p[j].d );
								p[j].normal()[k] = 0.0;
								snpd = true;
							}
						}
						if ( snpd ){
							VectorNormalize( p[j].normal() );
							//Sys_Printf( "nrm plane %6.8f %6.8f %6.8f %6.8f\n", p[j].a, p[j].b, p[j].c, p[j].d );
							p[j].dist() = vector3_dot( ( points[j] + points[( j + 1 ) % 3] ) / 2.0, p[j].normal() );
							//Sys_Printf( "dst plane %6.8f %6.8f %6.8f %6.8f\n", p[j].a, p[j].b, p[j].c, p[j].d );
						}
					}

					/* make back plane */
					reverse = plane3_flipped( plane );
					reverse.dist() += clipDepth;

					make_brush_sides<5>( plane, p, reverse, points, si );
				}


				else if ( spf == eExtrudeFaceNormals ){

					/* make side planes */
					for ( j = 0; j < 3; ++j )
					{
						p[j].normal() = VectorNormalized( vector3_cross( plane.normal(), points[( j + 1 ) % 3] - points[j] ) );
						p[j].dist() = vector3_dot( points[j], p[j].normal() );
						//snap nearly axial side planes
						snpd = false;
						for ( k = 0; k < 3; ++k )
						{
							if ( fabs( p[j].normal()[k] ) < 0.00025 && p[j].normal()[k] != 0.0 ){
								//Sys_Printf( "init plane %6.8f %6.8f %6.8f %6.8f\n", p[j].a, p[j].b, p[j].c, p[j].d );
								p[j].normal()[k] = 0.0;
								snpd = true;
							}
						}
						if ( snpd ){
							VectorNormalize( p[j].normal() );
							//Sys_Printf( "nrm plane %6.8f %6.8f %6.8f %6.8f\n", p[j].a, p[j].b, p[j].c, p[j].d );
							const Vector3 cnt = ( points[j] + points[( j + 1 ) % 3] ) / 2.0;
							p[j].dist() = vector3_dot( cnt, p[j].normal() );
							//Sys_Printf( "dst plane %6.8f %6.8f %6.8f %6.8f\n", p[j].a, p[j].b, p[j].c, p[j].d );
						}
					}

					/* make back plane */
					reverse = plane3_flipped( plane );
					reverse.dist() += clipDepth;

					nonax_clip_dbg( p );

					make_brush_sides<5>( plane, p, reverse, points, si );
				}


				else if ( spf == ePyramidaClip ){

					/* calculate center */
					Vector3 cnt = ( points[0] + points[1] + points[2] ) / 3.0;

					/* make back pyramid point */
					cnt -= plane.normal() * clipDepth;

					/* make 3 more planes */
					if( PlaneFromPoints( p[0], points[1], points[0], cnt ) &&
					    PlaneFromPoints( p[1], points[2], points[1], cnt ) &&
					    PlaneFromPoints( p[2], points[0], points[2], cnt ) ) {

						//check for dangerous planes
						while( (( p[0].a != 0.0 || p[0].b != 0.0 ) && fabs( p[0].a ) < 0.00025 && fabs( p[0].b ) < 0.00025) ||
						       (( p[0].a != 0.0 || p[0].c != 0.0 ) && fabs( p[0].a ) < 0.00025 && fabs( p[0].c ) < 0.00025) ||
						       (( p[0].c != 0.0 || p[0].b != 0.0 ) && fabs( p[0].c ) < 0.00025 && fabs( p[0].b ) < 0.00025) ||
						       (( p[1].a != 0.0 || p[1].b != 0.0 ) && fabs( p[1].a ) < 0.00025 && fabs( p[1].b ) < 0.00025) ||
						       (( p[1].a != 0.0 || p[1].c != 0.0 ) && fabs( p[1].a ) < 0.00025 && fabs( p[1].c ) < 0.00025) ||
						       (( p[1].c != 0.0 || p[1].b != 0.0 ) && fabs( p[1].c ) < 0.00025 && fabs( p[1].b ) < 0.00025) ||
						       (( p[2].a != 0.0 || p[2].b != 0.0 ) && fabs( p[2].a ) < 0.00025 && fabs( p[2].b ) < 0.00025) ||
						       (( p[2].a != 0.0 || p[2].c != 0.0 ) && fabs( p[2].a ) < 0.00025 && fabs( p[2].c ) < 0.00025) ||
						       (( p[2].c != 0.0 || p[2].b != 0.0 ) && fabs( p[2].c ) < 0.00025 && fabs( p[2].b ) < 0.00025) ) {
							cnt -= plane.normal() * 0.1f;
							//	Sys_Printf( "shifting pyramid point\n" );
							PlaneFromPoints( p[0], points[1], points[0], cnt );
							PlaneFromPoints( p[1], points[2], points[1], cnt );
							PlaneFromPoints( p[2], points[0], points[2], cnt );
						}

						nonax_clip_dbg( p );

						make_brush_sides<4>( plane, p, reverse, points, si );
					}
					else
					{
						Sys_Warning( "triangle (%6.0f %6.0f %6.0f) (%6.0f %6.0f %6.0f) (%6.0f %6.0f %6.0f) of %s was not autoclipped\n",
						             points[0][0], points[0][1], points[0][2], points[1][0], points[1][1], points[1][2], points[2][0], points[2][1], points[2][2], modelName );
						continue;
					}
				}


				else if ( ( si->clipModel && spf == 0 ) || ( spawnFlags & eClipFlags ) == eClipModel ){	//default CLIPMODEL

default_CLIPMODEL:
					// axial normal
					Vector3 bestNormal = plane.normal();
					normal_make_axial( bestNormal );

					/* make side planes */
					for ( j = 0; j < 3; ++j )
					{
						p[j].normal() = VectorNormalized( vector3_cross( bestNormal, points[( j + 1 ) % 3] - points[j] ) );
						p[j].dist() = vector3_dot( points[j], p[j].normal() );
					}

					/* make back plane */
					reverse = plane3_flipped( plane );
					reverse.dist() += vector3_dot( bestNormal, plane.normal() ) * clipDepth;

					nonax_clip_dbg( p );

					make_brush_sides<5>( plane, p, reverse, points, si );
				}


				/* add to entity */
				if ( CreateBrushWindings( buildBrush ) ) {
					AddBrushBevels();
					//%	EmitBrushes( buildBrush, NULL, NULL );
					brush_t& newBrush = entity.brushes.emplace_front( buildBrush );
					newBrush.original = &newBrush;
					entity.numBrushes++;
				}
				else{
					Sys_Warning( "triangle (%6.0f %6.0f %6.0f) (%6.0f %6.0f %6.0f) (%6.0f %6.0f %6.0f) of %s was not autoclipped\n",
					             points[0][0], points[0][1], points[0][2], points[1][0], points[1][1], points[1][2], points[2][0], points[2][1], points[2][2], modelName );
				}
			}
		}
		// normalEpsilon = normalEpsilon_save;
	}
	else if ( spawnFlags & eClipFlags ){
		Sys_Warning( "nonexistent clipping mode selected\n" );
	}
}
