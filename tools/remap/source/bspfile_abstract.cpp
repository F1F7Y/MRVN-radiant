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
#include <ctime>




/* -------------------------------------------------------------------------------

   this file was copied out of the common directory in order to not break
   compatibility with the q3map 1.x tree. it was moved out in order to support
   the raven bsp format (RBSP) used in soldier of fortune 2 and jedi knight 2.

   since each game has its own set of particular features, the data structures
   below no longer directly correspond to the binary format of a particular game.

   the translation will be done at bsp load/save time to keep any sort of
   special-case code messiness out of the rest of the program.

   ------------------------------------------------------------------------------- */


/*
   SwapBlock()
   if all values are 32 bits, this can be used to swap everything
*/
void SwapBlock(int *block, int size) {
    /* dummy check */
    if (block == NULL) {
        return;
    }

    /* swap */
    size >>= 2;
    for (int i = 0; i < size; ++i) {
        block[i] = LittleLong(block[i]);
    }
}


/*
   SwapBlock()
   if all values are 32 bits, this can be used to swap everything
*/
template<typename T>
void SwapBlock(std::vector<T> &block){
    const size_t size = (sizeof(T) * block.size()) >> 2;  // get size in integers
    /* swap */
    int *intptr = reinterpret_cast<int*>(block.data());
    for (size_t i = 0; i < size; ++i) {
        intptr[i] = LittleLong(intptr[i]);
    }
}


/*
   SwapBSPFile()
   byte swaps all data in the abstract bsp
*/
static void SwapBSPFile() {}


/*
   LoadEntFile()
   loads an ent file
*/
void LoadEntFile(const char *filename, std::vector<char> &ents) {
    if (!FileExists(filename)) {
        return;
    }
    /* Open it */
    MemBuffer f = LoadFile(filename);
    /* Add to bspEntities */
    std::vector<char> ent = { (char*)((byte*)f.data() + 10), (char*)((byte*)f.data() + f.size()) };
    ents.insert(ents.end(), ent.begin(), ent.end());
}


/*
   LoadBSPFile()
   loads a bsp file into memory
*/
void LoadBSPFile(const char *filename) {
    /* dummy check */
    if (g_game == NULL || g_game->load == NULL) {
        Error("LoadBSPFile: unsupported BSP file format");
    }

    /* load it, then byte swap the in-memory version */
    //g_game->load(filename);
    //SwapBSPFile();
}


/*
   LoadBSPFilePartially()
   loads bsp file parts meaningful for autopacker
*/
void LoadBSPFilePartially(const char *filename) {
    /* dummy check */
    if (g_game == NULL || g_game->load == NULL) {
        Error("LoadBSPFile: unsupported BSP file format");
    }

    /* load it, then byte swap the in-memory version */
    // g_game->load(filename);
    // LoadIBSPorRBSPFilePartially(filename);
    // SwapBSPFile();
}


/*
   WriteBSPFile()
   writes a bsp file
*/
void WriteBSPFile(const char *filename) {
    char tempname[1024];
    time_t tm;

    Sys_Printf("Writing %s... ", filename);

    /* dummy check */
    if (g_game == NULL || g_game->write == NULL) {
        Sys_Printf("Failed!\n");
        Error("WriteBSPFile: unsupported BSP file format");
    }

    Sys_Printf("Success!\n");

    /* make fake temp name so existing bsp file isn't damaged in case write process fails */
    time(&tm);
    sprintf(tempname, "%s.%08X", filename, (int)tm);

    /* byteswap, write the bsp, then swap back so it can be manipulated further */
    SwapBSPFile();
    g_game->write(tempname);
    SwapBSPFile();

    /* replace existing bsp file */
    remove(filename);
    rename(tempname, filename);
}


/*
    WriteEntFileHeader()
*/
void WriteEntFileHeader(FILE *file) {
    if (g_game->bspVersion == 47) {
        // Apex Legends
        std::string message = "ENTITIES02 num_models=" + std::to_string(ApexLegends::Bsp::levelInfo.at(0).modelCount) + "\n";
        SafeWrite(file, message.c_str(), message.size());
    } else {
        // Titanfall
        char message[] = "ENTITIES01\n";
        SafeWrite(file, &message, sizeof(message) - 1);
    }
}


/*
    WriteEntFiles()
    Writes .ent files used by .bsp
*/
void WriteEntFiles(const char *path) {
    // env
    if (Titanfall::Ent::env.size()) {
        auto name = StringOutputStream(1024)(PathExtensionless(path), "_env.ent");
        Sys_Printf("Writing %s... ", name.c_str());

        FILE *file = SafeOpenWrite(name);
        WriteEntFileHeader(file);
        Titanfall::Ent::env.emplace_back('\0');
        SafeWrite(file, Titanfall::Ent::env.data(), Titanfall::Ent::env.size());
        fclose(file);

        Sys_Printf("Success!\n");
    }
    // fx
    if (Titanfall::Ent::fx.size()) {
        auto name = StringOutputStream(1024)(PathExtensionless(path), "_fx.ent");
        Sys_Printf("Writing %s... ", name.c_str());

        FILE *file = SafeOpenWrite(name);
        WriteEntFileHeader(file);
        Titanfall::Ent::fx.emplace_back('\0');
        SafeWrite(file, Titanfall::Ent::fx.data(), Titanfall::Ent::fx.size());
        fclose(file);

        Sys_Printf("Success!\n");
    }
    // script
    if (Titanfall::Ent::script.size()) {
        auto name = StringOutputStream(1024)(PathExtensionless(path), "_script.ent");
        Sys_Printf("Writing %s... ", name.c_str());

        FILE *file = SafeOpenWrite(name);
        WriteEntFileHeader(file);
        Titanfall::Ent::script.emplace_back('\0');
        SafeWrite(file, Titanfall::Ent::script.data(), Titanfall::Ent::script.size());
        fclose(file);

        Sys_Printf("Success!\n");
    }
    // snd
    if (Titanfall::Ent::snd.size()) {
        auto name = StringOutputStream(1024)(PathExtensionless(path), "_snd.ent");
        Sys_Printf("Writing %s... ", name.c_str());

        FILE *file = SafeOpenWrite(name);
        WriteEntFileHeader(file);
        Titanfall::Ent::snd.emplace_back('\0');
        SafeWrite(file, Titanfall::Ent::snd.data(), Titanfall::Ent::snd.size());
        fclose(file);

        Sys_Printf("Success!\n");
    }
    // spawn
    if (Titanfall::Ent::spawn.size()) {
        auto name = StringOutputStream(1024)(PathExtensionless(path), "_spawn.ent");
        Sys_Printf("Writing %s... ", name.c_str());

        FILE *file = SafeOpenWrite(name);
        WriteEntFileHeader(file);
        Titanfall::Ent::spawn.emplace_back('\0');
        SafeWrite(file, Titanfall::Ent::spawn.data(), Titanfall::Ent::spawn.size());
        fclose(file);

        Sys_Printf("Success!\n");
    }
    // extra
    // has contents only when -onlyentities argument is used, doesn't get referenced by the BSP!
    if (Titanfall::Ent::extra.size()) {
        auto name = StringOutputStream(1024)(PathExtensionless(path), "_extra.ent");
        Sys_Printf("Writing %s... ", name.c_str());

        FILE* file = SafeOpenWrite(name);
        WriteEntFileHeader(file);
        Titanfall::Ent::extra.emplace_back('\0');
        SafeWrite(file, Titanfall::Ent::extra.data(), Titanfall::Ent::extra.size());
        fclose(file);

        Sys_Printf("Success!\n");
    }
}


/* -------------------------------------------------------------------------------

   entity data handling

   ------------------------------------------------------------------------------- */


/*
   StripTrailing()
   strips low byte chars off the end of a string
*/
inline StringRange StripTrailing(const char *string) {
    const char *end = string + strlen(string);
    while (end != string && end[-1] <= 32) {
        --end;
    }
    return StringRange(string, end);
}


/*
   ParseEpair()
   parses a single quoted "key" "value" pair into an epair struct
*/
void ParseEPair(std::list<epair_t> &epairs) {
    /* handle key */
    /* strip trailing spaces that sometimes get accidentally added in the editor */
    epair_t ep;
    ep.key = StripTrailing(token);

    /* handle value */
    GetToken(false);
    ep.value = StripTrailing(token);

    if(!ep.key.empty() && !ep.value.empty())
        epairs.push_back(std::move(ep));
}


/*
   ParseEntity()
   parses an entity's epairs
*/
static bool ParseEntity() {
    /* dummy check */
    if (!GetToken(true)) {
        return false;
    }

    if (!strEqual(token, "{")) {
        Error("ParseEntity: { not found");
    }

    /* create new entity */
    entity_t &e = entities.emplace_back();

    /* parse */
    while (1) {
        if (!GetToken(true)) {
            Error("ParseEntity: EOF without closing brace");
        }
        if (strEqual(token,"}")) {
            break;
        }
        ParseEPair(e.epairs);
    }

    /* return to sender */
    return true;
}


/*
   ParseEntities()
   parses the bsp entity data string into entities
*/
void ParseEntities() {
    entities.clear();
    ParseFromMemory(Titanfall::Bsp::entities.data(), Titanfall::Bsp::entities.size());
    while (ParseEntity()) {};

    /* ydnar: set number of bsp entities in case a map is loaded on top */
    numBSPEntities = entities.size();
}


/*
    must be called before UnparseEntities
*/
void InjectCommandLine(const char *stage, const std::vector<const char*> &args) {
    auto str = StringStream(entities[0].valueForKey("_q3map2_cmdline"));  // read previousCommandLine
    if(!str.empty()) {
        str << "; ";
    }

    str << stage;

    for (const char *c : args) {
        str << ' ';
        for(; !strEmpty(c); ++c) {
            if (*c != '\\' && *c != '"' && *c != ';' && (unsigned char)*c >= ' ') {
                str << *c;
            }
        }
    }

    entities[0].setKeyValue("_q3map2_cmdline", str);
    entities[0].setKeyValue("_q3map2_version", Q3MAP_VERSION);
}


/*
   UnparseEntities()
   generates the entdata string from all the entities.
   this allows the utilities to add or remove key/value
   pairs to the data created by the map editor
*/
void UnparseEntities() {
    StringOutputStream data(8192);
    for (std::size_t i = 0; i < numBSPEntities && i < entities.size(); ++i) {  // run through entity list
        const entity_t &e = entities[i];
        if (e.epairs.empty()) {
            continue;  // ent got removed
        }
        /* ydnar: certain entities get stripped from bsp file */
        const char *classname = e.classname();
        #define ENT_IS(x)  striEqual(classname, x)
        if (ENT_IS("prop_static")
         || ENT_IS("_decal")
         || ENT_IS("_skybox")) {
            continue;
        }
        #undef ENT_IS

        data << "{\n";
        for (const auto &ep : e.epairs) {
            data << '\"' << StripTrailing(ep.key.c_str()) << "\" \"" << StripTrailing(ep.value.c_str()) << "\"\n";
        }
        data << "}\n";
    }

    bspEntData = { data.begin(), data.end() + 1 };  // include '\0'
}


/*
   PrintEntity()
   prints an entity's epairs to the console
*/
void PrintEntity(const entity_t *ent) {
    Sys_Printf("------- entity %p -------\n", ent);
    for (const auto &ep : ent->epairs) {
        Sys_Printf("%s = %s\n", ep.key.c_str(), ep.value.c_str());
    }
}


/*
   setKeyValue()
   sets an epair in an entity
*/
void entity_t::setKeyValue(const char *key, const char *value) {
    /* check for existing epair */
    for (auto &ep : epairs) {
        if (EPAIR_EQUAL(ep.key.c_str(), key)) {
            ep.value = value;
            return;
        }
    }

    /* create new epair */
    epairs.emplace_back(epair_t{ key, value });
}


/*
   valueForKey()
   gets the value for an entity key
*/
const char *entity_t::valueForKey(const char *key) const {
    /* walk epair list */
    for (const auto &ep : epairs) {
        if (EPAIR_EQUAL(ep.key.c_str(), key)) {
            return ep.value.c_str();
        }
    }

    /* if no match, return empty string */
    return "";
}


bool entity_t::read_keyvalue_(bool &bool_value, std::initializer_list<const char*> &&keys) const {
    for (const char *key : keys) {
        const char *value = valueForKey(key);
        if (!strEmpty(value)) {
            bool_value = (value[0] == '1');
            return true;
        }
    }
    return false;
}


bool entity_t::read_keyvalue_(int &int_value, std::initializer_list<const char*> &&keys) const {
    for (const char *key : keys) {
        const char *value = valueForKey(key);
        if (!strEmpty(value)) {
            int_value = atoi(value);
            return true;
        }
    }
    return false;
}


bool entity_t::read_keyvalue_(float &float_value, std::initializer_list<const char*> &&keys) const {
    for (const char *key : keys) {
        const char *value = valueForKey(key);
        if(!strEmpty(value)) {
            float_value = atof(value);
            return true;
        }
    }
    return false;
}


bool entity_t::read_keyvalue_(Vector3 &vector3_value, std::initializer_list<const char*> &&keys) const {
    for (const char *key : keys) {
        const char *value = valueForKey(key);
        if (!strEmpty(value)) {
            float v0, v1, v2;
            if (3 == sscanf(value, "%f %f %f", &v0, &v1, &v2)) {
                vector3_value[0] = v0;
                vector3_value[1] = v1;
                vector3_value[2] = v2;
                return true;
            }
        }
    }
    return false;
}


bool entity_t::read_keyvalue_(const char *&string_ptr_value, std::initializer_list<const char*> &&keys) const {
    for (const char *key : keys) {
        const char *value = valueForKey(key);
        if (!strEmpty(value)) {
            string_ptr_value = value;
            return true;
        }
    }
    return false;
}


/*
   FindTargetEntity()
   finds an entity target
*/
entity_t *FindTargetEntity(const char *target) {
    /* walk entity list */
    for (auto &e : entities) {
        if (strEqual(e.valueForKey("targetname"), target)) {
            return &e;
        }
    }

    /* nada */
    return NULL;
}


/*
   GetEntityShadowFlags() - ydnar
   gets an entity's shadow flags
   NOTE: does not set them to defaults if the keys are not found!
*/
void GetEntityShadowFlags(const entity_t *ent, const entity_t *ent2, int *castShadows, int *recvShadows) {
    /* get cast shadows */
    if (castShadows != NULL) {
        (ent != NULL && ent->read_keyvalue(*castShadows, "_castShadows", "_cs")) ||
        (ent2 != NULL && ent2->read_keyvalue(*castShadows, "_castShadows", "_cs"));
    }

    /* receive */
    if (recvShadows != NULL) {
        (ent != NULL && ent->read_keyvalue(*recvShadows, "_receiveShadows", "_rs")) ||
        (ent2 != NULL && ent2->read_keyvalue(*recvShadows, "_receiveShadows", "_rs"));
    }

    /* vortex: game-specific default entity keys */
    if (striEqual(g_game->magic, "dq") || striEqual(g_game->magic, "prophecy")) {
        /* vortex: deluxe quake default shadow flags */
        if (ent->classname_is("func_wall")) {
            if (recvShadows != NULL) {
                *recvShadows = 1;
            }
            if (castShadows != NULL) {
                *castShadows = 1;
            }
        }
    }
}
