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
#include "bspfile_abstract.h"

/*
   EmitShader()
   emits a bsp shader entry
 */

int EmitShader( const char *shader, const int *contentFlags, const int *surfaceFlags ){
	shaderInfo_t    *si;


	/* handle special cases */
	if ( shader == NULL ) {
		shader = "noshader";
	}

	/* try to find an existing shader */
	for ( size_t i = 0; i < bspShaders.size(); ++i )
	{
		/* ydnar: handle custom surface/content flags */
		if ( surfaceFlags != NULL && bspShaders[ i ].surfaceFlags != *surfaceFlags ) {
			continue;
		}
		if ( contentFlags != NULL && bspShaders[ i ].contentFlags != *contentFlags ) {
			continue;
		}
		if ( !doingBSP ){
			si = ShaderInfoForShader( shader );
			if ( !strEmptyOrNull( si->remapShader ) ) {
				shader = si->remapShader;
			}
		}
		/* compare name */
		if ( striEqual( shader, bspShaders[ i ].shader ) ) {
			return i;
		}
	}

	/* get shaderinfo */
	si = ShaderInfoForShader( shader );

	/* emit a new shader */
	const int i = bspShaders.size(); // store index
	bspShader_t& bspShader = bspShaders.emplace_back();

	strcpy( bspShader.shader, si->shader );
	/* handle custom content/surface flags */
	bspShader.surfaceFlags = ( surfaceFlags != NULL )? *surfaceFlags : si->surfaceFlags;
	bspShader.contentFlags = ( contentFlags != NULL )? *contentFlags : si->contentFlags;

	/* recursively emit any damage shaders */
	if ( !strEmptyOrNull( si->damageShader ) ) {
		Sys_FPrintf( SYS_VRB, "Shader %s has damage shader %s\n", si->shader.c_str(), si->damageShader );
		EmitShader( si->damageShader, NULL, NULL );
	}

	/* return index */
	return i;
}



/*
   EmitPlanes()
   there is no opportunity to discard planes, because all of the original
   brushes will be saved in the map
 */

static void EmitPlanes(){
	bspPlanes.reserve( mapplanes.size() );
	/* walk plane list */
	for ( const plane_t& plane : mapplanes )
	{
		bspPlanes.push_back( plane.plane );
	}

	/* emit some statistics */
	Sys_FPrintf( SYS_VRB, "%9zu BSP planes\n", bspPlanes.size() );
}



/*
   EmitLeaf()
   emits a leafnode to the bsp file
 */

static void EmitLeaf( node_t *node ){
	bspLeaf_t& leaf = bspLeafs.emplace_back();

	leaf.cluster = node->cluster;
	leaf.area = node->area;

	/* emit bounding box */
	leaf.minmax.maxs = node->minmax.maxs;
	leaf.minmax.mins = node->minmax.mins;

	/* emit leaf brushes */
	leaf.firstBSPLeafBrush = bspLeafBrushes.size();
	for ( const brush_t& b : node->brushlist )
	{
		bspLeafBrushes.push_back( b.original->outputNum );
	}

	leaf.numBSPLeafBrushes = bspLeafBrushes.size() - leaf.firstBSPLeafBrush;

	/* emit leaf surfaces */
	if ( node->opaque ) {
		return;
	}

	/* add the drawSurfRef_t drawsurfs */
	leaf.firstBSPLeafSurface = bspLeafSurfaces.size();
	for ( const drawSurfRef_t *dsr = node->drawSurfReferences; dsr; dsr = dsr->nextRef )
	{
		bspLeafSurfaces.push_back( dsr->outputNum );
	}

	leaf.numBSPLeafSurfaces = bspLeafSurfaces.size() - leaf.firstBSPLeafSurface;
}


/*
   EmitDrawNode_r()
   recursively emit the bsp nodes
 */

static int EmitDrawNode_r( node_t *node ){
	/* check for leafnode */
	if ( node->planenum == PLANENUM_LEAF ) {
		EmitLeaf( node );
		return -int( bspLeafs.size() );
	}

	/* emit a node */
	const int id = bspNodes.size();
	{
		bspNode_t& bnode = bspNodes.emplace_back();

		bnode.minmax.mins = node->minmax.mins;
		bnode.minmax.maxs = node->minmax.maxs;

		if ( node->planenum & 1 ) {
			Error( "WriteDrawNodes_r: odd planenum" );
		}
		bnode.planeNum = node->planenum;
	}

	//
	// recursively output the other nodes
	//
	for ( int i = 0; i < 2; i++ )
	{
		// reference node by id, as it may be reallocated
		if ( node->children[i]->planenum == PLANENUM_LEAF ) {
			bspNodes[id].children[i] = -int( bspLeafs.size() + 1 );
			EmitLeaf( node->children[i] );
		}
		else
		{
			bspNodes[id].children[i] = bspNodes.size();
			EmitDrawNode_r( node->children[i] );
		}
	}

	return id;
}



/*
   ============
   SetModelNumbers
   ============
 */
void SetModelNumbers(){
	int models = 1;
	for ( std::size_t i = 1; i < entities.size(); ++i ) {
		if ( !entities[i].brushes.empty() || entities[i].patches ) {
			char value[16];
			sprintf( value, "*%i", models );
			models++;
			entities[i].setKeyValue( "model", value );
		}
	}

}




/*
   SetLightStyles()
   sets style keys for entity lights
 */

void SetLightStyles(){
	int j, numStyles;
	char value[ 10 ];
	char lightTargets[ MAX_SWITCHED_LIGHTS ][ 64 ];
	int lightStyles[ MAX_SWITCHED_LIGHTS ];
	int numStrippedLights = 0;

	/* -keeplights option: force lights to be kept and ignore what the map file says */
	if ( keepLights ) {
		entities[0].setKeyValue( "_keepLights", "1" );
	}

	/* ydnar: determine if we keep lights in the bsp */
	entities[ 0 ].read_keyvalue( keepLights, "_keepLights" );

	/* any light that is controlled (has a targetname) must have a unique style number generated for it */
	numStyles = 0;
	for ( std::size_t i = 1; i < entities.size(); ++i )
	{
		entity_t& e = entities[ i ];

		if ( !e.classname_prefixed( "light" ) ) {
			continue;
		}
		const char *t;
		if ( !e.read_keyvalue( t, "targetname" ) ) {
			/* ydnar: strip the light from the BSP file */
			if ( !keepLights ) {
				e.epairs.clear();
				numStrippedLights++;
			}

			/* next light */
			continue;
		}

		/* get existing style */
		const int style = e.intForKey( "style" );
		if ( style < LS_NORMAL || style > LS_NONE ) {
			Error( "Invalid lightstyle (%d) on entity %zu", style, i );
		}

		/* find this targetname */
		for ( j = 0; j < numStyles; j++ )
			if ( lightStyles[ j ] == style && strEqual( lightTargets[ j ], t ) ) {
				break;
			}

		/* add a new style */
		if ( j >= numStyles ) {
			if ( numStyles == MAX_SWITCHED_LIGHTS ) {
				Error( "MAX_SWITCHED_LIGHTS (%d) exceeded, reduce the number of lights with targetnames", MAX_SWITCHED_LIGHTS );
			}
			strcpy( lightTargets[ j ], t );
			lightStyles[ j ] = style;
			numStyles++;
		}

		/* set explicit style */
		sprintf( value, "%d", 32 + j );
		e.setKeyValue( "style", value );

		/* set old style */
		if ( style != LS_NORMAL ) {
			sprintf( value, "%d", style );
			e.setKeyValue( "switch_style", value );
		}
	}

	/* emit some statistics */
	//Sys_FPrintf( SYS_VRB, "%9d light entities stripped\n", numStrippedLights );
}



/*
   BeginBSPFile()
   starts a new bsp file
 */

void BeginBSPFile(){
}



/*
   EndBSPFile()
   finishes a new bsp and writes to disk
 */

void EndBSPFile( bool do_write ){
	Sys_FPrintf( SYS_VRB, "--- EndBSPFile ---\n" );

	numBSPEntities = entities.size();
	UnparseEntities();

	if ( do_write ) {
		/* write the surface extra file */
		//WriteSurfaceExtraFile( source );

		/* write the bsp */
		auto path = StringStream( source, ".bsp" );
		WriteBSPFile( path );
		WriteEntFiles( source );
	}
}
