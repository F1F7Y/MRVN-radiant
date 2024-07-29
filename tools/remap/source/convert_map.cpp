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


/*
   ConvertBrush()
   exports a map brush
 */

inline float Det3x3( float a00, float a01, float a02,
                     float a10, float a11, float a12,
                     float a20, float a21, float a22 ){
	return
	        a00 * ( a11 * a22 - a12 * a21 )
	    -   a01 * ( a10 * a22 - a12 * a20 )
	    +   a02 * ( a10 * a21 - a11 * a20 );
}

static void GetBestSurfaceTriangleMatchForBrushside( const side_t& buildSide, const bspDrawVert_t *bestVert[3] ){
}

#define FRAC( x ) ( ( x ) - floor( x ) )


static void bspBrush_to_buildBrush( const bspBrush_t& brush ){
	/* clear out build brush 
	buildBrush.sides.clear();

	bool modelclip = false;
	// try to guess if thats model clip 
	if ( force ){
		int notNoShader = 0;
		modelclip = true;
		for ( int i = 0; i < brush.numSides; i++ )
		{
			// get side 
			const bspBrushSide_t& side = bspBrushSides[ brush.firstSide + i ];

			// get shader 
			if ( side.shaderNum < 0 || side.shaderNum >= int( bspShaders.size() ) ) {
				continue;
			}
			const bspShader_t& shader = bspShaders[ side.shaderNum ];
			//"noshader" happens on modelclip and unwanted sides ( usually breaking complex brushes )
			if( !striEqual( shader.shader, "noshader" ) ){
				notNoShader++;
			}
			if( notNoShader > 1 ){
				modelclip = false;
				break;
			}
		}
	}

	// iterate through bsp brush sides 
	for ( int i = 0; i < brush.numSides; i++ )
	{
		// get side 
		const bspBrushSide_t& side = bspBrushSides[ brush.firstSide + i ];

		// get shader 
		if ( side.shaderNum < 0 || side.shaderNum >= int( bspShaders.size() ) ) {
			continue;
		}
		const bspShader_t& shader = bspShaders[ side.shaderNum ];
		//"noshader" happens on modelclip and unwanted sides ( usually breaking complex brushes )
		if( striEqual( shader.shader, "default" ) || ( striEqual( shader.shader, "noshader" ) && !modelclip ) )
			continue;

		// add build side 
		buildBrush.sides.emplace_back();

		// tag it 
		buildBrush.sides.back().shaderInfo = ShaderInfoForShader( shader.shader );
		buildBrush.sides.back().planenum = side.planeNum;
	}
	*/
}




#undef FRAC

#if 0
/* iterate through the brush sides (ignore the first 6 bevel planes) */
for ( i = 0; i < brush->numSides; i++ )
{
	/* get side */
	side = &bspBrushSides[ brush->firstSide + i ];

	/* get shader */
	if ( side->shaderNum < 0 || side->shaderNum >= int( bspShaders.size() ) ) {
		continue;
	}
	shader = &bspShaders[ side->shaderNum ];
	if ( striEqual( shader->shader, "default" ) || striEqual( shader->shader, "noshader" ) ) {
		continue;
	}

	/* get texture name */
	if ( striEqualPrefix( shader->shader, "textures/" ) ) {
		texture = shader->shader + 9;
	}
	else{
		texture = shader->shader;
	}

	/* get plane */
	plane = &bspPlanes[ side->planeNum ];

	/* make plane points */
	{
		vec3_t vecs[ 2 ];


		MakeNormalVectors( plane->normal, vecs[ 0 ], vecs[ 1 ] );
		VectorMA( vec3_origin, plane->dist, plane->normal, pts[ 0 ] );
		VectorMA( pts[ 0 ], 256.0f, vecs[ 0 ], pts[ 1 ] );
		VectorMA( pts[ 0 ], 256.0f, vecs[ 1 ], pts[ 2 ] );
	}

	/* offset by origin */
	for ( j = 0; j < 3; j++ )
		VectorAdd( pts[ j ], origin, pts[ j ] );

	/* print brush side */
	/* ( 640 24 -224 ) ( 448 24 -224 ) ( 448 -232 -224 ) common/caulk 0 48 0 0.500000 0.500000 0 0 0 */
	fprintf( f, "\t\t( %.3f %.3f %.3f ) ( %.3f %.3f %.3f ) ( %.3f %.3f %.3f ) %s 0 0 0 0.5 0.5 0 0 0\n",
	         pts[ 0 ][ 0 ], pts[ 0 ][ 1 ], pts[ 0 ][ 2 ],
	         pts[ 1 ][ 0 ], pts[ 1 ][ 1 ], pts[ 1 ][ 2 ],
	         pts[ 2 ][ 0 ], pts[ 2 ][ 1 ], pts[ 2 ][ 2 ],
	         texture );
}
#endif



/*
   ConvertPatch()
   converts a bsp patch to a map patch

    {
        patchDef2
        {
            base_wall/concrete
            ( 9 3 0 0 0 )
            (
                ( ( 168 168 -192 0 2 ) ( 168 168 -64 0 1 ) ( 168 168 64 0 0 ) ... )
                ...
            )
        }
    }

 */




/*
   ConvertEPairs()
   exports entity key/value pairs to a map file
 */

static void ConvertEPairs( FILE *f, const entity_t& e, bool skip_origin ){
	/* walk epairs */
	for ( const auto& ep : e.epairs )
	{
		/* ignore empty keys/values */
		if ( ep.key.empty() || ep.value.empty() ) {
			continue;
		}

		/* ignore model keys with * prefixed values */
		if ( striEqual( ep.key.c_str(), "model" ) && ep.value.c_str()[ 0 ] == '*' ) {
			continue;
		}

		/* ignore origin keys if skip_origin is set */
		if ( skip_origin && striEqual( ep.key.c_str(), "origin" ) ) {
			continue;
		}

		/* emit the epair */
		fprintf( f, "\t\"%s\" \"%s\"\n", ep.key.c_str(), ep.value.c_str() );
	}
}


/*
   ConvertBSPToMap()
   exports an quake map file from the bsp
 */

static int ConvertBSPToMap_Ext( char *bspName, bool brushPrimitives )
{
	// Note it 
	Sys_FPrintf( SYS_VRB, "--- Convert BSP to MAP ---\n" );

	// Create map filename from the bsp name
	const auto name = StringStream( PathExtensionless( bspName ), "_converted.map" );
	Sys_Printf( "writing %s\n", name.c_str() );

	// Open map file
	FILE* f = SafeOpenWrite(name);

	// Print header
	fprintf(f, "// Generated by remap (F1FTY) -convert -format map\n");

	// Loop through all entities
	for( std::size_t i = 0; i < entities.size(); i++ ) {
		const entity_t &entity = entities.at( i );

		// Start entity
		fprintf(f, "// entity %zu\n", i);
		fprintf(f, "// brushes %zu\n", entity.brushes.size());
		fprintf(f, "{\n");

		// Save its keyvalues
		ConvertEPairs(f, entity, false);
		fprintf(f, "\n");

		// Save brushes
		std::size_t j = 0;
		for( const brush_t &brush : entity.brushes ) {
			// Start brush entry
			fprintf(f, "// brush %llu\n", j);
			fprintf(f, "{\n");
			fprintf(f, "brushDef\n");
			fprintf(f, "{\n");

			// Write planes 
			for( const side_t &side : brush.sides ) {
				const Plane3 &plane = side.plane;

				Vector3 pts[3];
				{
					Vector3 vecs[2];
					MakeNormalVectors(plane.normal(), vecs[0], vecs[1]);
					pts[0] = plane.normal() * plane.dist();
					pts[1] = pts[0] + vecs[0] * 64.0f;
					pts[2] = pts[0] + vecs[1] * 64.0f;
				}

				fprintf(f, "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) ( ( %.7f %.7f %.7f ) ( %.7f %.7f %.7f ) ) %s %d 0 0\n",
					(int)pts[0][0], (int)pts[0][1], (int)pts[0][2],
					(int)pts[1][0], (int)pts[1][1], (int)pts[1][2],
					(int)pts[2][0], (int)pts[2][1], (int)pts[2][2],
					1.0f / 32.0f, 0.0f, 0.0f,
					0.0f, 1.0f / 32.0f, 0.0f,
					side.shaderInfo->shader.c_str(),
					0
				);
			}

			// Close brush entry
			fprintf(f, "}\n");
			fprintf(f, "}\n");

			j++;
		}

		// Save patches
		parseMesh_t* patch;
		patch = entity.patches;
		while( patch != NULL ) {
			
			mesh_t patchMesh = patch->mesh;
			
			fprintf(f, "// patch\n");
			fprintf(f, "{\n");
			fprintf(f, "patchDef2\n");
			fprintf(f, "{\n");
			fprintf(f, "%s\n", patch->shaderInfo->shader.c_str());
			fprintf(f, "( %i %i %i %i %i )\n", patchMesh.width, patchMesh.height, 0, 0, 0);

			fprintf(f, "(\n");

			for( int x = 0; x < patchMesh.width; x++ ) {
				fprintf(f, "( ");
				for( int y = 0; y < patchMesh.height; y++ ) {
					bspDrawVert_t &vert = patchMesh.verts[x * patchMesh.height + y];
					fprintf(f, "( %f %f %f %f %f ) ", vert.xyz[0], vert.xyz[1], vert.xyz[2], vert.st[0], vert.st[1]);
				}
				fprintf(f, ")\n");
			}

			fprintf(f, ")\n");

			fprintf(f, "}\n");
			fprintf(f, "}\n");
			
			patch = patch->next;
		}

		// Close entity
		fprintf(f, "}\n");
	}

	// Close the file and return
	fclose(f);

	// Return to sender
	return 0;
}

int ConvertBSPToMap( char *bspName ){
	return ConvertBSPToMap_Ext( bspName, false );
}

int ConvertBSPToMap_BP( char *bspName ){
	return ConvertBSPToMap_Ext( bspName, true );
}
