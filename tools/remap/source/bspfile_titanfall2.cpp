/* -------------------------------------------------------------------------------

   My feeble attempt at generating a r2 ( Titanfall 2 ) .bsp file.

   A lot of this is temporary and will be altered heavily with new information.
   If you know how to c++ better than me feel free to contribute or call me words
   in discord. :)

   - Fifty#8113, also known as Fifteen, Sixteen, Seventeen, Eigtheen and FORTY

   ------------------------------------------------------------------------------- */



/* dependencies */
#include "remap.h"
#include "bspfile_abstract.h"
#include <ctime>


/* constants */
#define R2_LUMP_ENTITIES							0x00
#define R2_LUMP_PLANES								0x01
#define R2_LUMP_TEXTURE_DATA						0x02
#define R2_LUMP_VERTICES							0x03
#define R2_LUMP_LIGHTPROBE_PARENT_INFOS				0x04
#define R2_LUMP_SHADOW_ENVIRONMENTS					0x05
#define R2_LUMP_LIGHTPROBE_BSP_NODES				0x06
#define R2_LUMP_LIGHTPROBE_BSP_REF_IDS				0x07
#define R2_LUMP_MODELS								0x0E
#define R2_LUMP_ENTITY_PARTITIONS					0x18
#define R2_LUMP_PHYSICS_COLLIDE						0x1D
#define R2_LUMP_VERTEX_NORMALS						0x1E
#define R2_LUMP_GAME_LUMP							0x23
#define R2_LUMP_LEAF_WATER_DATA						0x24
#define R2_LUMP_PAKFILE								0x28
#define R2_LUMP_CUBEMAPS							0x2A
#define R2_LUMP_TEXTURE_DATA_STRING_DATA			0x2B
#define R2_LUMP_TEXTURE_DATA_STRING_TABLE			0x2C
#define R2_LUMP_WORLD_LIGHTS						0x36
#define R2_LUMP_WORLD_LIGHT_PARENT_INFOS			0x37
#define R2_LUMP_PHYSICS_LEVEL						0x3E
#define R2_LUMP_TRICOLL_TRIS						0x42
#define R2_LUMP_TRICOLL_NODES						0x44
#define R2_LUMP_TRICOLL_HEADERS						0x45
#define R2_LUMP_PHYSICS_TRIANGLES					0x46
#define R2_LUMP_VERTEX_UNLIT						0x47
#define R2_LUMP_VERTEX_LIT_FLAT						0x48
#define R2_LUMP_VERTEX_LIT_BUMP						0x49
#define R2_LUMP_VERTEX_UNLIT_TS						0x4A
#define R2_LUMP_VERTEX_BLINN_PHONG					0x4B
#define R2_LUMP_VERTEX_RESERVED_5					0x4C
#define R2_LUMP_VERTEX_RESERVED_6					0x4D
#define R2_LUMP_VERTEX_RESERVED_7					0x4E
#define R2_LUMP_MESH_INDICES						0x4F
#define R2_LUMP_MESHES								0x50
#define R2_LUMP_MESH_BOUNDS							0x51
#define R2_LUMP_MATERIAL_SORT						0x52
#define R2_LUMP_LIGHTMAP_HEADERS					0x53
#define R2_LUMP_CM_GRID								0x55
#define R2_LUMP_CM_GRID_CELLS						0x56
#define R2_LUMP_CM_GRID_SETS						0x57
#define R2_LUMP_CM_GEO_SET_BOUNDS					0x58
#define R2_LUMP_CM_PRIMITIVES						0x59
#define R2_LUMP_CM_PRIMITIVE_BOUNDS					0x5A
#define R2_LUMP_CM_UNIQUE_CONTENTS					0x5B
#define R2_LUMP_CM_BRUSHES							0x5C
#define R2_LUMP_CM_BRUSH_SIDE_PLANES				0x5D
#define R2_LUMP_CM_BRUSH_SIDE_PROPS					0x5E
#define R2_LUMP_CM_BRUSH_TEX_VECS					0x5F
#define R2_LUMP_TRICOLL_BEVEL_STARTS				0x60
#define R2_LUMP_TRICOLL_BEVEL_INDICES				0x61
#define R2_LUMP_LIGHTMAP_DATA_SKY					0x62
#define R2_LUMP_CSM_AABB_NODES						0x63
#define R2_LUMP_CSM_OBJ_REFERENCES					0x64
#define R2_LUMP_LIGHTPROBES							0x65
#define R2_LUMP_STATIC_PROP_LIGHTPROBE_INDICES		0x66
#define R2_LUMP_LIGHTPROBE_TREE						0x67
#define R2_LUMP_LIGHTPROBE_REFERENCES				0x68
#define R2_LUMP_LIGHTMAP_DATA_REAL_TIME_LIGHTS		0x69
#define R2_LUMP_CELL_BSP_NODES						0x6A
#define R2_LUMP_CELLS								0x6B
#define R2_LUMP_PORTALS								0x6C
#define R2_LUMP_PORTAL_VERTICES						0x6D
#define R2_LUMP_PORTAL_EDGES						0x6E
#define R2_LUMP_PORTAL_VERTEX_EDGES					0x6F
#define R2_LUMP_PORTAL_VERTEX_REFERENCES			0x70
#define R2_LUMP_PORTAL_EDGE_REGERENCES				0x71
#define R2_LUMP_PORTAL_EDGE_INTERSECT_EDGE			0x72
#define R2_LUMP_PORTAL_EDGE_INTERSECT_AT_VERTEX		0x73
#define R2_LUMP_PORTAL_EDGE_INTERSECT_HEADER		0x74
#define R2_LUMP_OCCLUSION_MESH_VERTICES				0x75
#define R2_LUMP_OCCLUSION_MESH_INDICES				0x76
#define R2_LUMP_CELL_AABB_NODES						0x77
#define R2_LUMP_OBJ_REFERENCES						0x78
#define R2_LUMP_OBJ_REFERENCE_BOUNDS				0x79
#define R2_LUMP_LIGHTMAP_DATA_RTL_PAGE				0x7A
#define R2_LUMP_LEVEL_INFO							0x7B
#define R2_LUMP_SHADOW_MESH_OPAQUE_VERTICES			0x7C
#define R2_LUMP_SHADOW_MESH_ALPHA_VERTICES			0x7D
#define R2_LUMP_SHADOW_MESH_INDICES					0x7E
#define R2_LUMP_SHADOW_MESHES						0x7F




/* funcs */

/*
   LoadR2BSPFile()
   loads a titanfall2 bsp file
 */
void LoadR2BSPFile(const char* filename)
{
	Sys_FPrintf( SYS_VRB, "Loading bsp file: \"%s\"\n", filename );
	
	// Load file into memory
	MemBuffer file = LoadFile(filename);

	rbspHeader_t* header = file.data();

	// Make sure magic matches the format we're trying to load
	if ( !force && memcmp( header->ident, g_game->bspIdent, 4 ) ) {
		Error("%s is not a %s file", filename, g_game->bspIdent);
	}
	// TODO: Make this game agnostic so we can for example import tfo maps while apex legends is our game
	if ( !force && header->version != g_game->bspVersion ) {
		Error("%s is version %d, not %d", filename, header->version, g_game->bspVersion);
	}

	// Load lumps
	CopyLump( (rbspHeader_t*) header, R2_LUMP_ENTITIES,				Titanfall::Bsp::entities );
	CopyLump( (rbspHeader_t*) header, R2_LUMP_PLANES,				Titanfall::Bsp::planes );
	CopyLump( (rbspHeader_t*) header, R2_LUMP_ENTITY_PARTITIONS,	Titanfall::Bsp::entityPartitions );
	CopyLump( (rbspHeader_t*) header, R2_LUMP_CM_GRID,				Titanfall::Bsp::cmGrid );
	CopyLump( (rbspHeader_t*) header, R2_LUMP_CM_BRUSHES,			Titanfall::Bsp::cmBrushes );
	CopyLump( (rbspHeader_t*) header, R2_LUMP_CM_BRUSH_SIDE_PLANES, Titanfall::Bsp::cmBrushSidePlaneOffsets);
	CopyLump( (rbspHeader_t*) header, R2_LUMP_CM_BRUSH_SIDE_PROPS,	Titanfall::Bsp::cmBrushSideProperties );
	

	// Load all .ent files referenced in bsp if they exist
	// TODO: Actually do this ^
	#if 1
	if ( header->lumps[R2_LUMP_ENTITY_PARTITIONS].length )
	{
		/* Spawn */
		{
			auto name = StringOutputStream(256)(PathExtensionless(filename), "_spawn.ent");
			LoadEntFile( name.c_str(), Titanfall::Bsp::entities );
		}
		/* Snd */
		{
			auto name = StringOutputStream(256)(PathExtensionless(filename), "_snd.ent");
			LoadEntFile( name.c_str(), Titanfall::Bsp::entities );
		}
		/* Script */
		{
			auto name = StringOutputStream(256)(PathExtensionless(filename), "_script.ent");
			LoadEntFile(name.c_str(), Titanfall::Bsp::entities);
		}
		/* Fx */
		{
			auto name = StringOutputStream(256)(PathExtensionless(filename), "_fx.ent");
			LoadEntFile(name.c_str(), Titanfall::Bsp::entities);
		}
		/* Env */
		{
			auto name = StringOutputStream(256)(PathExtensionless(filename), "_env.ent");
			LoadEntFile(name.c_str(), Titanfall::Bsp::entities);
		}
	}
	#endif


	// Parse entities into entities vector
	ParseEntities();
	
	// Make worldspawn if we have 0 entities
	// This should never happen
	if ( entities.size() == 0 ) {
		entity_t &worldspawn = entities.emplace_back();
		worldspawn.setKeyValue( "classname", "worldspawn" );
	}

	for ( Titanfall::CMBrush_t &bspBrush : Titanfall::Bsp::cmBrushes ) {
		Vector3 mins = bspBrush.origin - bspBrush.extents;
		Vector3 maxs = bspBrush.origin + bspBrush.extents;
		
		// Create the base 6 planes from the brush AABB 
		std::vector<Plane3> planes;
		{
			Vector3 vertices[8];
			vertices[0] = maxs;
			vertices[1] = Vector3(maxs.x(), maxs.y(), mins.z());
			vertices[2] = Vector3(maxs.x(), mins.y(), maxs.z());
			vertices[3] = Vector3(mins.x(), maxs.y(), maxs.z());
			vertices[4] = mins;
			vertices[5] = Vector3(mins.x(), mins.y(), maxs.z());
			vertices[6] = Vector3(mins.x(), maxs.y(), mins.z());
			vertices[7] = Vector3(maxs.x(), mins.y(), mins.z());

			// Create planes from AABB
			Plane3& plane0 = planes.emplace_back();
			PlaneFromPoints(plane0, vertices[0], vertices[1], vertices[2]);
			Plane3& plane1 = planes.emplace_back();
			PlaneFromPoints(plane1, vertices[0], vertices[2], vertices[3]);
			Plane3& plane2 = planes.emplace_back();
			PlaneFromPoints(plane2, vertices[0], vertices[3], vertices[1]);
			Plane3& plane3 = planes.emplace_back();
			PlaneFromPoints(plane3, vertices[6], vertices[5], vertices[4]);
			Plane3& plane4 = planes.emplace_back();
			PlaneFromPoints(plane4, vertices[7], vertices[6], vertices[4]);
			Plane3& plane5 = planes.emplace_back();
			PlaneFromPoints(plane5, vertices[5], vertices[7], vertices[4]);
		}
		
		// NOTE: This doesnt work :)
		// Add extra planes to brush
		/*for (uint16_t i = 0; i < bspBrush.planeCount; i++) {
			if ( bspBrush.sidePlaneIndex + Titanfall::Bsp::cmBrushSidePlaneOffsets.at( bspBrush.sidePlaneIndex ) + Titanfall::Bsp::cmGrid[0].brushSidePlaneOffset > 72 )
				break;
			
			Plane3 &plane = planes.emplace_back();
			plane = Plane3( Titanfall::Bsp::planes.at( bspBrush.sidePlaneIndex + Titanfall::Bsp::cmBrushSidePlaneOffsets.at( bspBrush.sidePlaneIndex ) + Titanfall::Bsp::cmGrid[0].brushSidePlaneOffset ) );
		}*/

		// Make brush and fill it with sides
		brush_t &brush = entities.data()[0].brushes.emplace_back();
		
		for ( Plane3 plane : planes )
		{
			side_t& side = brush.sides.emplace_back();
			side.plane = plane;

			String512 textureName;
			textureName = "NULL";

			side.shaderInfo = new shaderInfo_t;
			side.shaderInfo->shader = textureName;
		}
	}
}


/*
   WriteR2BSPFile()
   writes a titanfall2 bsp file and it's .ent files
 */
void WriteR2BSPFile(const char* filename)
{
	rbspHeader_t header{};


	/* set up header */
	memcpy(header.ident, g_game->bspIdent, 4);
	header.version = LittleLong(g_game->bspVersion);
	header.mapVersion = 30;
	header.maxLump = 127;

	/* This should be automated maybe */
	header.lumps[0x01].lumpVer = 1;
	header.lumps[0x02].lumpVer = 1;
	header.lumps[0x36].lumpVer = 3;
	header.lumps[0x42].lumpVer = 2;
	header.lumps[0x44].lumpVer = 1;
	header.lumps[0x45].lumpVer = 1;
	header.lumps[0x49].lumpVer = 1;
	header.lumps[0x53].lumpVer = 1;

	/* write initial header */
	FILE* file = SafeOpenWrite(filename);
	SafeWrite(file, &header, sizeof(header));    /* overwritten later */


	/* :) */
	{
		char message[64] = "Built with love using MRVN-radiant :)";
		SafeWrite(file, &message, sizeof(message));
	}
	{
		char message[64];
		strncpy(message,StringOutputStream(64)("Version:        ", Q3MAP_VERSION).c_str(),64);
		SafeWrite(file, &message, sizeof(message));
	}
	{
		time_t t;
		time(&t);
		char message[64];
		strncpy(message,StringOutputStream(64)("Time:           ", asctime(localtime(&t))).c_str(),64);
		SafeWrite(file, &message, sizeof(message));
	}
	
	/* Write lumps */
	
	AddLump(file, header.lumps[R2_LUMP_ENTITIES],							Titanfall::Bsp::entities);
	AddLump(file, header.lumps[R2_LUMP_PLANES],								Titanfall::Bsp::planes);
	AddLump(file, header.lumps[R2_LUMP_TEXTURE_DATA],						Titanfall::Bsp::textureData);
	AddLump(file, header.lumps[R2_LUMP_VERTICES],							Titanfall::Bsp::vertices);
	AddLump(file, header.lumps[R2_LUMP_MODELS],								Titanfall::Bsp::models);
	AddLump(file, header.lumps[R2_LUMP_VERTEX_NORMALS],						Titanfall::Bsp::vertexNormals);
	AddLump(file, header.lumps[R2_LUMP_ENTITY_PARTITIONS],					Titanfall::Bsp::entityPartitions);
	/* Game Lump */
	{
		std::size_t start = ftell(file);
		header.lumps[R2_LUMP_GAME_LUMP].offset = start;
		header.lumps[R2_LUMP_GAME_LUMP].length = 36 + Titanfall2::GameLump.pathCount * sizeof(Titanfall2::GameLump_Path) + Titanfall2::GameLump.propCount * sizeof(Titanfall2::GameLump_Prop);
		Titanfall2::GameLump.offset = start + 20;
		Titanfall2::GameLump.length = 16 + Titanfall2::GameLump.pathCount * sizeof(Titanfall2::GameLump_Path) + Titanfall2::GameLump.propCount * sizeof(Titanfall2::GameLump_Prop);
		SafeWrite(file, &Titanfall2::GameLump, sizeof(Titanfall2::GameLump));
		/* need to write vectors separately */
		/* paths */
		fseek(file, start + 24, SEEK_SET);
		SafeWrite(file, Titanfall2::GameLump.paths.data(), 128 * Titanfall2::GameLump.pathCount);
		/* :) */
		SafeWrite(file, &Titanfall2::GameLump.propCount, 4);
		SafeWrite(file, &Titanfall2::GameLump.propCount, 4);
		SafeWrite(file, &Titanfall2::GameLump.propCount, 4);
		/* props */
		SafeWrite(file, Titanfall2::GameLump.props.data(), 64 * Titanfall2::GameLump.propCount);
		SafeWrite(file, &Titanfall2::GameLump.unk5, 4);
	}
	AddLump(file, header.lumps[R2_LUMP_TEXTURE_DATA_STRING_DATA],			Titanfall::Bsp::textureDataData);
	AddLump(file, header.lumps[R2_LUMP_TEXTURE_DATA_STRING_TABLE],			Titanfall::Bsp::textureDataTable);
	AddLump(file, header.lumps[R2_LUMP_WORLD_LIGHTS],						Titanfall2::Bsp::worldLights_stub);
	//AddLump(file, header.lumps[R2_LUMP_TRICOLL_TRIS],						Titanfall2::bspTricollTris_stub);
	//AddLump(file, header.lumps[R2_LUMP_TRICOLL_NODES],					Titanfall2::bspTricollNodes_stub);
	//AddLump(file, header.lumps[R2_LUMP_TRICOLL_HEADERS],					Titanfall2::bspTricollHeaders_stub);
	AddLump(file, header.lumps[R2_LUMP_VERTEX_UNLIT],						Titanfall::Bsp::vertexUnlitVertices);
	AddLump(file, header.lumps[R2_LUMP_VERTEX_LIT_FLAT],					Titanfall::Bsp::vertexLitFlatVertices);
	AddLump(file, header.lumps[R2_LUMP_VERTEX_LIT_BUMP],					Titanfall::Bsp::vertexLitBumpVertices);
	AddLump(file, header.lumps[R2_LUMP_VERTEX_UNLIT_TS],					Titanfall::Bsp::vertexUnlitTSVertices);
	AddLump(file, header.lumps[R2_LUMP_VERTEX_BLINN_PHONG],					Titanfall::Bsp::vertexBlinnPhongVertices);
	AddLump(file, header.lumps[R2_LUMP_MESH_INDICES],						Titanfall::Bsp::meshIndices);
	AddLump(file, header.lumps[R2_LUMP_MESHES],								Titanfall::Bsp::meshes);
	AddLump(file, header.lumps[R2_LUMP_MESH_BOUNDS],						Titanfall::Bsp::meshBounds);
	AddLump(file, header.lumps[R2_LUMP_MATERIAL_SORT],						Titanfall::Bsp::materialSorts);
	AddLump(file, header.lumps[R2_LUMP_LIGHTMAP_HEADERS],					Titanfall2::Bsp::lightMapHeaders_stub);
	AddLump(file, header.lumps[R2_LUMP_CM_GRID],							Titanfall::Bsp::cmGrid);
	AddLump(file, header.lumps[R2_LUMP_CM_GRID_CELLS],						Titanfall::Bsp::cmGridCells);
	AddLump(file, header.lumps[R2_LUMP_CM_GRID_SETS],						Titanfall::Bsp::cmGeoSets);
	AddLump(file, header.lumps[R2_LUMP_CM_GEO_SET_BOUNDS],					Titanfall::Bsp::cmGeoSetBounds);
	//AddLump(file, header.lumps[R2_LUMP_CM_PRIMITIVES],					Titanfall::Bsp::cmPrimitives_stub);
	//AddLump(file, header.lumps[R2_LUMP_CM_PRIMITIVE_BOUNDS],				Titanfall::Bsp::cmPrimitiveBounds_stub);
	AddLump(file, header.lumps[R2_LUMP_CM_UNIQUE_CONTENTS],					Titanfall::Bsp::cmUniqueContents_stub);
	AddLump(file, header.lumps[R2_LUMP_CM_BRUSHES],							Titanfall::Bsp::cmBrushes);
	AddLump(file, header.lumps[R2_LUMP_CM_BRUSH_SIDE_PROPS],				Titanfall::Bsp::cmBrushSideProperties);
	AddLump(file, header.lumps[R2_LUMP_CM_BRUSH_SIDE_PLANES],				Titanfall::Bsp::cmBrushSidePlaneOffsets);
	//AddLump(file, header.lumps[R2_LUMP_TRICOLL_BEVEL_STARTS],				Titanfall2::bspTricollBevelStarts_stub);
	AddLump(file, header.lumps[R2_LUMP_LIGHTMAP_DATA_SKY],					Titanfall2::Bsp::lightMapDataSky_stub);
	AddLump(file, header.lumps[R2_LUMP_CSM_AABB_NODES],						Titanfall::Bsp::csmAABBNodes_stub);
	AddLump(file, header.lumps[R2_LUMP_CELL_BSP_NODES],						Titanfall::Bsp::cellBSPNodes_stub);
	AddLump(file, header.lumps[R2_LUMP_CELLS],								Titanfall::Bsp::cells_stub);
	AddLump(file, header.lumps[R2_LUMP_CELL_AABB_NODES],					Titanfall::Bsp::cellAABBNodes);
	AddLump(file, header.lumps[R2_LUMP_OBJ_REFERENCES],						Titanfall::Bsp::objReferences);
	AddLump(file, header.lumps[R2_LUMP_OBJ_REFERENCE_BOUNDS],				Titanfall::Bsp::objReferenceBounds);
	AddLump(file, header.lumps[R2_LUMP_LEVEL_INFO],							Titanfall::Bsp::levelInfo);


	/* emit bsp size */
	const int size = ftell(file);
	Sys_Printf("Wrote %.1f MB (%d bytes)\n", (float)size / (1024 * 1024), size);

	/* write the completed header */
	fseek(file, 0, SEEK_SET);
	SafeWrite(file, &header, sizeof(header));

	/* close the file */
	fclose(file);
}

/*
   CompileR2BSPFile()
   writes a titanfall2 bsp file and it's .ent files
 */
void CompileR2BSPFile()
{
	SetUpGameLump();

	for (size_t entityNum = 0; entityNum < entities.size(); ++entityNum)
	{
		/* get entity */
		entity_t& entity = entities[entityNum];
		const char* classname = entity.classname();

		/* visible geo */
		if ( striEqual( classname,"worldspawn" ) )
		{
			Titanfall::BeginModel();
			/* generate bsp meshes from map brushes */
			Shared::MakeMeshes( entity );
			Titanfall::EmitMeshes( entity );
			
			Titanfall::EmitBrushes( entity );

			Titanfall::EndModel();
		}
		/* hurt */
		else if ( striEqualPrefix( classname, "trigger_" ) )
		{
			Titanfall::EmitTriggerBrushPlaneKeyValues( entity );
		}
		/* props for gamelump */
		else if ( striEqual( classname, "misc_model" ) )
		{
			EmitProp( entity );
		}


		Titanfall::EmitEntity( entity );
	}

	/* */
	Titanfall::EmitEntityPartitions();

	Titanfall::EmitCollisionGrid();

	/**/
	Shared::MakeVisReferences();
	Shared::visRoot = Shared::MakeVisTree( Shared::visRefs, 1e30f );
	Titanfall::EmitVisTree();

	/* Emit LevelInfo */
	Titanfall::EmitLevelInfo();

	/* Generate unknown lumps */
	Titanfall2::EmitStubs();
	Titanfall::EmitStubs();
}

/*
	EmitStubs
	Fills out all the nessesary lumps we dont generate so the map at least boots and we can noclip around
*/
void Titanfall2::EmitStubs()
{
	/* World Lights */
	{
		constexpr std::array<uint8_t, 1008> data = {
			0x00, 0x00, 0xC0, 0x42, 0x00, 0x00, 0x10, 0x42, 0x00, 0x80, 0x82, 0x43, 0xCE, 0xCC, 0x4C, 0x3E,
			0xCE, 0xCC, 0x4C, 0x3E, 0xCE, 0xCC, 0x4C, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xDF, 0x00, 0xC0, 0xC2, 0x99, 0xFB, 0x0F, 0xC2, 0x00, 0x80, 0x82, 0x43, 0xCE, 0xCC, 0x4C, 0x3E,
			0xCE, 0xCC, 0x4C, 0x3E, 0xCE, 0xCC, 0x4C, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0xC0, 0x42, 0x00, 0x00, 0x10, 0x42, 0x00, 0x80, 0x82, 0x43, 0x00, 0x00, 0x80, 0x40,
			0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x40, 0x1D, 0xBF, 0xAB, 0xBE, 0x92, 0x8B, 0x06, 0x3F,
			0xFC, 0x25, 0x48, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xDF, 0x00, 0xC0, 0xC2, 0x99, 0xFB, 0x0F, 0xC2, 0x00, 0x80, 0x82, 0x43, 0x00, 0x00, 0x80, 0x40,
			0x00, 0x00, 0x80, 0x40, 0x00, 0x00, 0x80, 0x40, 0xC7, 0xEB, 0x8B, 0x3E, 0xEB, 0xD3, 0x05, 0xBF,
			0x62, 0xB7, 0x4E, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x60, 0xC1, 0x00, 0x00, 0xC2, 0x42, 0x3A, 0x19, 0x00, 0x46,
			0x4B, 0x16, 0xC3, 0x46, 0xF6, 0x55, 0x08, 0x47, 0x22, 0xD1, 0x18, 0x3E, 0x41, 0x84, 0x52, 0xBF,
			0x41, 0x91, 0x0C, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB2, 0x8F, 0x70, 0x3F,
			0x43, 0x1D, 0xAF, 0x3E, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0x80, 0x3F,
			0x65, 0xF0, 0x1F, 0xBC, 0x8F, 0xBA, 0xC7, 0x37, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
			0x00, 0x00, 0x18, 0xC2, 0x00, 0x00, 0x85, 0x43, 0x00, 0x00, 0x8E, 0x43, 0xA5, 0x7B, 0x20, 0x47,
			0x27, 0x68, 0xF4, 0x47, 0x7E, 0xCD, 0x2A, 0x48, 0x26, 0x86, 0x6A, 0x3E, 0xD4, 0x96, 0x2B, 0xBF,
			0x60, 0xB5, 0x34, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB2, 0x8F, 0x70, 0x3F,
			0x43, 0x1D, 0xAF, 0x3E, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0xAF, 0x43, 0x00, 0x00, 0x80, 0x3F,
			0x98, 0xC9, 0xB6, 0xBB, 0x63, 0x6F, 0x02, 0x37, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
			0x00, 0x00, 0x2A, 0x43, 0x00, 0x00, 0x56, 0x43, 0x00, 0x00, 0xB4, 0xC2, 0x34, 0x9D, 0xA4, 0x3E,
			0xC6, 0x6B, 0xE7, 0x3E, 0x00, 0xFF, 0xFE, 0x3E, 0x92, 0x2A, 0xB6, 0xBE, 0x60, 0x46, 0x13, 0xBF,
			0x07, 0x8C, 0x3C, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB2, 0x8F, 0x70, 0x3F,
			0x43, 0x1D, 0xAF, 0x3E, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0xC8, 0x43, 0x00, 0x00, 0x80, 0x3F,
			0x54, 0xA9, 0xF9, 0xB8, 0x8F, 0xBA, 0xC7, 0xB6, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
			0x00, 0x00, 0x98, 0xC1, 0x00, 0x00, 0xD8, 0x41, 0x00, 0x00, 0x98, 0xC1, 0x01, 0x40, 0x1C, 0x47,
			0x3C, 0x95, 0xDF, 0x46, 0x06, 0x81, 0x88, 0x46, 0x64, 0xF5, 0x1F, 0x3F, 0xD0, 0x05, 0xFD, 0x3E,
			0x2B, 0xBD, 0x1A, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D, 0x1B, 0x44, 0x3F,
			0xBA, 0x8D, 0x24, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x96, 0x43, 0x00, 0x00, 0x80, 0x3F,
			0xE3, 0x70, 0x26, 0xB9, 0x63, 0x89, 0x31, 0xB7, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F,
			0x00, 0x00, 0x92, 0xC2, 0x00, 0x00, 0x4C, 0xC3, 0x00, 0x00, 0x80, 0x41, 0x01, 0x50, 0x43, 0x48,
			0x05, 0xD1, 0x04, 0x48, 0x47, 0xA1, 0xAA, 0x47, 0xEE, 0x1C, 0x0B, 0x3F, 0xD0, 0xB3, 0x32, 0x3F,
			0x14, 0xBD, 0xEE, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D, 0x1B, 0x44, 0x3F,
			0xBA, 0x8D, 0x24, 0x3F, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x5E, 0x43, 0x00, 0x00, 0x80, 0x3F,
			0x94, 0xEB, 0x60, 0xB9, 0xB5, 0x1A, 0xA2, 0xB7, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F
		};
		Titanfall2::Bsp::worldLights_stub = { data.begin(), data.end() };
	}
	/* LightMap Headers */
	{
		constexpr std::array<uint8_t, 8> data = {
			0x01, 0x00, 0x00, 0x00, 0x00, 0x02, 0x80, 0x00
		};
		Titanfall2::Bsp::lightMapHeaders_stub = { data.begin(), data.end() };
	}
	/* LightMap Data Sky */
	{
		for (std::size_t i = 0; i < 524288; i++)
		{
			Titanfall2::Bsp::lightMapDataSky_stub.emplace_back(0);
		}
	}
}