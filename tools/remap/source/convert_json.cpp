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

   -------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */



/* dependencies */
#include "remap.h"
#define RAPIDJSON_WRITE_DEFAULT_FLAGS kWriteNanAndInfFlag
#define RAPIDJSON_PARSE_DEFAULT_FLAGS ( kParseCommentsFlag | kParseTrailingCommasFlag | kParseNanAndInfFlag )
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

#include <map>

#define for_indexed(...) for_indexed_v(i, __VA_ARGS__)
#define for_indexed_v(v, ...) if (std::size_t v = -1) for (__VA_ARGS__) if ((++v, true))


template<typename T, typename Allocator>
rapidjson::Value value_for( const BasicVector2<T>& vec, Allocator& allocator ){
	rapidjson::Value value( rapidjson::kArrayType );
	for( size_t i = 0; i != 2; ++i )
		value.PushBack( vec[i], allocator );
	return value;
}
template<typename T, typename Allocator>
rapidjson::Value value_for( const BasicVector3<T>& vec, Allocator& allocator ){
	rapidjson::Value value( rapidjson::kArrayType );
	for( size_t i = 0; i != 3; ++i )
		value.PushBack( vec[i], allocator );
	return value;
}
template<typename T, typename Allocator>
rapidjson::Value value_for( const BasicVector4<T>& vec, Allocator& allocator ){
	rapidjson::Value value( rapidjson::kArrayType );
	for( size_t i = 0; i != 4; ++i )
		value.PushBack( vec[i], allocator );
	return value;
}
template<typename T, size_t N, typename Allocator>
rapidjson::Value value_for( const T (&array)[N], Allocator& allocator ){
	rapidjson::Value value( rapidjson::kArrayType );
	for( auto&& val : array )
		value.PushBack( value_for( val, allocator ), allocator );
	return value;
}
template<typename T, size_t N, typename Allocator>
rapidjson::Value value_for_array( const T (&array)[N], Allocator& allocator ){
	rapidjson::Value value( rapidjson::kArrayType );
	for( auto&& val : array )
		value.PushBack( val, allocator );
	return value;
}

template<typename T>
void value_to( const rapidjson::Value& value, BasicVector2<T>& vec ){
	for( size_t i = 0; i != 2; ++i )
		vec[i] = value.GetArray().operator[]( i ).Get<T>();
}
template<typename T>
void value_to( const rapidjson::Value& value, BasicVector3<T>& vec ){
	for( size_t i = 0; i != 3; ++i )
		if constexpr ( std::is_same_v<T, byte> )
			vec[i] = value.GetArray().operator[]( i ).Get<int>();
		else
			vec[i] = value.GetArray().operator[]( i ).Get<T>();
}
template<typename T>
void value_to( const rapidjson::Value& value, BasicVector4<T>& vec ){
	for( size_t i = 0; i != 4; ++i )
		if constexpr ( std::is_same_v<T, byte> )
			vec[i] = value.GetArray().operator[]( i ).Get<int>();
		else
			vec[i] = value.GetArray().operator[]( i ).Get<T>();
}
template<typename T, size_t N>
void value_to( const rapidjson::Value& value, T (&array)[N] ){
	for_indexed( auto&& val : array )
		value_to( value.GetArray().operator[]( i ), val );
}
template<typename T, size_t N>
void value_to_array( const rapidjson::Value& value, T (&array)[N] ){
	for_indexed( auto&& val : array ){
		if constexpr ( std::is_same_v<T, byte> )
			val = value.GetArray().operator[]( i ).Get<int>();
		else
			val = value.GetArray().operator[]( i ).Get<T>();
	}
}


static void write_doc( const char *filename, rapidjson::Document& doc ){
	rapidjson::StringBuffer buffer;
   	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( buffer );
	writer.SetFormatOptions( rapidjson::kFormatSingleLineArray );
   	doc.Accept( writer );
	Sys_Printf( "Writing %s\n", filename );
	SaveFile( filename, buffer.GetString(), buffer.GetSize() );
}

static void write_json( const char *directory ){
	
}

inline rapidjson::Document load_json( const char *fileName ){
	Sys_Printf( "Loading %s\n", fileName );
	rapidjson::Document doc;
	doc.Parse( (const char*)LoadFile( fileName ).data() );
	ENSURE( !doc.HasParseError() );
	return doc;
}

static void read_json( const char *directory, bool useFlagNames, bool skipUnknownFlags ){
	
}

int ConvertJsonMain( Args& args ){
	/* arg checking */
	if ( args.empty() ) {
		Sys_Printf( "Usage: q3map2 -json <-unpack|-pack [-useflagnames[-skipflags]]> [-v] <mapname>\n" );
		return 0;
	}

	bool doPack = false; // unpack by default
	bool useFlagNames = false;
	bool skipUnknownFlags = false;

	/* process arguments */
	const char *fileName = args.takeBack();
	{
		while ( args.takeArg( "-pack" ) ) {
			doPack = true;
		}
		while ( args.takeArg( "-useflagnames" ) ) {
			useFlagNames = true;
			Sys_Printf( "Deducing surface/content flag values from their names in shaders.json\n" );
		}
		while ( args.takeArg( "-skipflags" ) ) {
			skipUnknownFlags = true;
			Sys_Printf( "Skipping unknown surface/content flag names\n" );
		}
	}

	// LoadShaderInfo();

	/* clean up map name */
	strcpy( source, ExpandArg( fileName ) );

	if( !doPack ){ // unpack
		path_set_extension( source, ".bsp" );
		Sys_Printf( "Loading %s\n", source );
		LoadBSPFile( source );
		ParseEntities();
		write_json( StringStream( PathExtensionless( source ), '/' ) );
	}
	else{
		/* write bsp */
		read_json( StringStream( PathExtensionless( source ), '/' ), useFlagNames, skipUnknownFlags );
		UnparseEntities();
		path_set_extension( source, "_json.bsp" );
		Sys_Printf( "Writing %s\n", source );
		WriteBSPFile( source );
	}

	/* return to sender */
	return 0;
}
