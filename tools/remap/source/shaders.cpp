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
#include "shaders.h"


static bool           g_warnImage = true;
static surfaceParm_t  custSurfaceParms[256];
static int            numCustSurfaceParms;


/*
   ColorMod()
   routines for dealing with vertex color/alpha modification
*/
void ColorMod(const colorMod_t *colormod, int numVerts, bspDrawVert_t *drawVerts) {
    /* dummy check */
    if (colormod == NULL || numVerts < 1 || drawVerts == NULL) {
        return;
    }

    /* walk vertex list */
    for (bspDrawVert_t &dv : Span(drawVerts, numVerts)) {
        /* walk colorMod list */
        for (const colorMod_t *cm = colormod; cm != NULL; cm = cm->next) {
            float c;
            /* default */
            Color4f mult(1, 1, 1, 1);
            Color4f  add(0, 0, 0, 0);

            const Vector3 cm_vec3 = vector3_from_array(cm->data);
            /* switch on type */
            switch (cm->type) {
                case EColorMod::ColorSet:
                    mult.rgb().set(0);
                    add.rgb() = cm_vec3 * 255.0f;
                    break;

                case EColorMod::AlphaSet:
                    mult.alpha() = 0.0f;
                    add.alpha() = cm->data[0] * 255.0f;
                    break;

                case EColorMod::ColorScale:
                    mult.rgb() = cm_vec3;
                    break;

                case EColorMod::AlphaScale:
                    mult.alpha() = cm->data[0];
                    break;

                case EColorMod::ColorDotProduct:
                    c = vector3_dot(dv.normal, cm_vec3);
                    mult.rgb().set(c);
                    break;

                case EColorMod::ColorDotProductScale:
                    c = vector3_dot(dv.normal, cm_vec3);
                    c = (c - cm->data[3]) / (cm->data[4] - cm->data[3]);
                    mult.rgb().set(c);
                    break;

                case EColorMod::AlphaDotProduct:
                    mult.alpha() = vector3_dot(dv.normal, cm_vec3);
                    break;

                case EColorMod::AlphaDotProductScale:
                    c = vector3_dot(dv.normal, cm_vec3);
                    c = (c - cm->data[3]) / (cm->data[4] - cm->data[3]);
                    mult.alpha() = c;
                    break;

                case EColorMod::ColorDotProduct2:
                    c = vector3_dot(dv.normal, cm_vec3);
                    c *= c;
                    mult.rgb().set(c);
                    break;

                case EColorMod::ColorDotProduct2Scale:
                    c = vector3_dot(dv.normal, cm_vec3);
                    c *= c;
                    c = (c - cm->data[3]) / (cm->data[4] - cm->data[3]);
                    mult.rgb().set(c);
                    break;

                case EColorMod::AlphaDotProduct2:
                    mult.alpha() = vector3_dot(dv.normal, cm_vec3);
                    mult.alpha() *= mult.alpha();
                    break;

                case EColorMod::AlphaDotProduct2Scale:
                    c = vector3_dot(dv.normal, cm_vec3);
                    c *= c;
                    c = (c - cm->data[3]) / (cm->data[4] - cm->data[3]);
                    mult.alpha() = c;
                    break;

                default:
                    break;
            }

            /* apply mod */
            for (auto &color : dv.color) {
                color = color_to_byte(mult * static_cast<BasicVector4<byte>>(color) + add);
            }
        }
    }
}


/*
   TCMod*()
   routines for dealing with a 3x3 texture mod matrix
*/
void TCMod(const tcMod_t &mod, Vector2 &st) {
    const Vector2 old = st;

    st[0] = (mod[0][0] * old[0]) + (mod[0][1] * old[1]) + mod[0][2];
    st[1] = (mod[1][0] * old[0]) + (mod[1][1] * old[1]) + mod[1][2];
}


static void TCModIdentity(tcMod_t &mod) {
    mod[0][0] = 1.0f;  mod[0][1] = 0.0f;  mod[0][2] = 0.0f;
    mod[1][0] = 0.0f;  mod[1][1] = 1.0f;  mod[1][2] = 0.0f;
    mod[2][0] = 0.0f;  mod[2][1] = 0.0f;  mod[2][2] = 1.0f;
    /* ^^^ this row is only used for multiples, not transformation ^^^ */
}


static void TCModMultiply(const tcMod_t &a, const tcMod_t &b, tcMod_t &out) {
    for (int i = 0; i < 3; i++) {
        out[i][0] = (a[i][0] * b[0][0]) + (a[i][1] * b[1][0]) + (a[i][2] * b[2][0]);
        out[i][1] = (a[i][0] * b[0][1]) + (a[i][1] * b[1][1]) + (a[i][2] * b[2][1]);
        out[i][2] = (a[i][0] * b[0][2]) + (a[i][1] * b[1][2]) + (a[i][2] * b[2][2]);
    }
}


static void TCModTranslate(tcMod_t &mod, float s, float t) {
    mod[0][2] += s;
    mod[1][2] += t;
}


static void TCModScale(tcMod_t &mod, float s, float t) {
    mod[0][0] *= s;
    mod[1][1] *= t;
}


static void TCModRotate(tcMod_t &mod, float euler) {
    tcMod_t  old, temp;
    float    radians, sinv, cosv;

    memcpy(old, mod, sizeof(tcMod_t));
    TCModIdentity(temp);

    radians = degrees_to_radians(euler);
    sinv = sin(radians);
    cosv = cos(radians);

    temp[0][0] =  cosv;  temp[0][1] = -sinv;
    temp[1][0] =  sinv;  temp[1][1] =  cosv;

    TCModMultiply(old, temp, mod);
}

/*
    GetShaderType()
    Returns a ShaderType_t corresponding to name
*/
const ShaderType_t *GetShaderType( const char *name ) {
    // Walk the current game's shader type vector
    for( const ShaderType_t &st : g_game->shaderTypes ) {
        if ( striEqual(name, st.name) ) {
            return &st;
        }
    }

    return nullptr;
}

/*
    ApplyShaderType()
    Applies a shader type to the supplied flags
*/
bool ApplyShaderType( const char *name, int *surfaceFlags, int *contentFlags, int *compileFlags ) {
    if( const ShaderType_t *st = GetShaderType(name) ) {
        // Clear and apply flags
        if( surfaceFlags != nullptr ) {
            *surfaceFlags &= ~(st->surfaceFlagsClear);
            *surfaceFlags |= st->surfaceFlags;
        }
        if( contentFlags != nullptr ) {
            *contentFlags &= ~(st->contentFlagsClear);
            *contentFlags |= st->contentFlags;
        }
        if( compileFlags != nullptr ) {
            *compileFlags &= ~(st->compileFlagsClear);
            *compileFlags |= st->compileFlags;
        }

        return true;
    }
    // Didn't find the shader type
    return false;
}

/*
    GetShaderFlag()
    Returns a ShaderType_t corresponding to name
*/
ShaderFlag_t *GetShaderFlag( const char *name, std::vector<ShaderFlag_t> flags ) {
    // Walk the current game's shader type vector
    for( ShaderFlag_t &sf : flags ) {
        if ( striEqual(name, sf.name) ) {
            return &sf;
        }
    }

    return nullptr;
}

/*
    ApplyShaderFlag()
    Applies a shader flag to the supplied flags
*/
bool ApplyShaderFlag( const char *name, int *surfaceFlags, int *contentFlags, int *compileFlags ) {
    ShaderFlag_t *sf = nullptr;
    if( surfaceFlags != nullptr ) { sf = GetShaderFlag(name, g_game->surfaceFlags); }
    if( contentFlags != nullptr ) { sf = GetShaderFlag(name, g_game->contentFlags); }
    if( compileFlags != nullptr ) { sf = GetShaderFlag(name, g_game->compileFlags); }


    if( sf != nullptr ) {
        // Clear and apply flags
        if( surfaceFlags != nullptr ) {
            *surfaceFlags &= ~(sf->flagsClear);
            *surfaceFlags |= sf->flags;
        }
        if( contentFlags != nullptr ) {
            *contentFlags &= ~(sf->flagsClear);
            *contentFlags |= sf->flags;
        }
        if( compileFlags != nullptr ) {
            *compileFlags &= ~(sf->flagsClear);
            *compileFlags |= sf->flags;
        }

        return true;
    }
    // Didn't find the shader type
    return false;
}


/*
   BeginMapShaderFile() - ydnar
   erases and starts a new map shader script
*/
void BeginMapShaderFile(const char *mapFile) {
    /* dummy check */
    mapName.clear();
    mapShaderFile = "";
    if (strEmptyOrNull(mapFile)) {
        return;
    }

    /* extract map name */
    mapName(PathFilename(mapFile));

    /* append ../scripts/q3map2_<mapname>.shader */
    mapShaderFile = StringStream(PathFilenameless(mapFile), "../", g_game->shaderPath,
                                            "/q3map2_", mapName.c_str(), ".shader");
    Sys_FPrintf(SYS_VRB, "Map has shader script %s\n", mapShaderFile.c_str());

    /* remove it */
    remove(mapShaderFile.c_str());

    /* stop making warnings about missing images */
    g_warnImage = false;
}


/*
   WriteMapShaderFile() - ydnar
   writes a shader to the map shader script
*/
void WriteMapShaderFile() {
    /* dummy check */
    if (mapShaderFile.empty()) {
        return;
    }

    /* are there any custom shaders? */
    if (std::none_of(shaderInfo, shaderInfo + numShaderInfo, [](const shaderInfo_t &si) { return si.custom; })) {
        return;
    }

    /* note it */
    Sys_FPrintf(SYS_VRB, "--- WriteMapShaderFile ---\n");
    Sys_FPrintf(SYS_VRB, "Writing %s", mapShaderFile.c_str());

    /* open shader file */
    FILE *file = fopen(mapShaderFile.c_str(), "wt");
    if (file == NULL) {
        Sys_Warning("Unable to open map shader file %s for writing\n", mapShaderFile.c_str());
        return;
    }

    /* print header */
    fprintf(file,
            "// Custom shader file for %s.bsp\n"
            "// Generated by Q3Map2 (ydnar)\n"
            "// Do not edit! This file is overwritten on recompiles.\n\n",
            mapName.c_str());

    /* walk the shader list */
    int num = 0;
    for (const shaderInfo_t &si : Span(shaderInfo, numShaderInfo)) {
        if (si.custom && !strEmptyOrNull(si.shaderText)) {
            num++;

            /* print it to the file */
            fprintf(file, "%s%s\n", si.shader.c_str(), si.shaderText);
            // Sys_Printf("%s%s\n", si.shader.c_str(), si.shaderText);
            /* FIXME: remove debugging code */

            Sys_FPrintf(SYS_VRB, ".");
        }
    }

    /* close the shader */
    fflush(file);
    fclose(file);

    Sys_FPrintf(SYS_VRB, "\n");

    /* print some stats */
    Sys_Printf("%9d custom shaders emitted\n", num);
}


/*
   CustomShader() - ydnar
   sets up a custom map shader
*/
const shaderInfo_t *CustomShader(const shaderInfo_t *si, const char *find, char *replace) {
    shaderInfo_t *csi;
    char          shader[MAX_QPATH];
    char         *s;
    int           loc;
    byte          digest[16];
    char         *srcShaderText, temp[8192], shaderText[8192];   /* ydnar: fixme (make this bigger?) */

    /* dummy check */
    if (si == NULL) {
        return ShaderInfoForShader("default");
    }

    /* default shader text source */
    srcShaderText = si->shaderText;

    /* et: implicitMap */
    if (si->implicitMap == EImplicitMap::Opaque) {
        srcShaderText = temp;
        sprintf(temp, "\n"
                      "{  // Q3Map2 defaulted (implicitMap)\n"
                      "\t{\n"
                      "\t\tmap $lightmap\n"
                      "\t\trgbGen identity\n"
                      "\t}\n"
                      "\tq3map_styleMarker\n"
                      "\t{\n"
                      "\t\tmap %s\n"
                      "\t\tblendFunc GL_DST_COLOR GL_ZERO\n"
                      "\t\trgbGen identity\n"
                      "\t}\n"
                      "}\n",
                si->implicitImagePath.c_str());
    } else if (si->implicitMap == EImplicitMap::Masked) {  /* et: implicitMask */
        srcShaderText = temp;
        sprintf(temp, "\n"
                      "{  // Q3Map2 defaulted (implicitMask)\n"
                      "\tcull none\n"
                      "\t{\n"
                      "\t\tmap %s\n"
                      "\t\talphaFunc GE128\n"
                      "\t\tdepthWrite\n"
                      "\t}\n"
                      "\t{\n"
                      "\t\tmap $lightmap\n"
                      "\t\trgbGen identity\n"
                      "\t\tdepthFunc equal\n"
                      "\t}\n"
                      "\tq3map_styleMarker\n"
                      "\t{\n"
                      "\t\tmap %s\n"
                      "\t\tblendFunc GL_DST_COLOR GL_ZERO\n"
                      "\t\tdepthFunc equal\n"
                      "\t\trgbGen identity\n"
                      "\t}\n"
                      "}\n",
                si->implicitImagePath.c_str(),
                si->implicitImagePath.c_str());
    } else if (si->implicitMap == EImplicitMap::Blend) {  /* et: implicitBlend */
        srcShaderText = temp;
        sprintf(temp, "\n"
                      "{  // Q3Map2 defaulted (implicitBlend)\n"
                      "\tcull none\n"
                      "\t{\n"
                      "\t\tmap %s\n"
                      "\t\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA\n"
                      "\t}\n"
                      "\t{\n"
                      "\t\tmap $lightmap\n"
                      "\t\trgbGen identity\n"
                      "\t\tblendFunc GL_DST_COLOR GL_ZERO\n"
                      "\t}\n"
                      "\tq3map_styleMarker\n"
                      "}\n",
                si->implicitImagePath.c_str());
    } else if (srcShaderText == NULL) {  /* default shader text */
        srcShaderText = temp;
        sprintf(temp, "\n"
                      "{  // Q3Map2 defaulted\n"
                      "\t{\n"
                      "\t\tmap $lightmap\n"
                      "\t\trgbGen identity\n"
                      "\t}\n"
                      "\tq3map_styleMarker\n"
                      "\t{\n"
                      "\t\tmap %s.tga\n"
                      "\t\tblendFunc GL_DST_COLOR GL_ZERO\n"
                      "\t\trgbGen identity\n"
                      "\t}\n"
                      "}\n",
                si->shader.c_str());
    }

    /* error check */
    if ((strlen(mapName) + 1 + 32) >= MAX_QPATH) {
        Error("Custom shader name length (%d) exceeded. Shorten your map name.\n", MAX_QPATH - 1);
    }

    /* do some bad find-replace */
    s = strIstr(srcShaderText, find);
    if (s == NULL) {
        // %  strcpy(shaderText, srcShaderText);
        return si;  /* testing just using the existing shader if this fails */
    } else {  /* substitute 'find' with 'replace' */
        loc = s - srcShaderText;
        strcpy(shaderText, srcShaderText);
        shaderText[loc] = '\0';
        strcat(shaderText, replace);
        strcat(shaderText, &srcShaderText[loc + strlen(find)]);
    }

    /* make md4 hash of the shader text */
    Com_BlockFullChecksum(shaderText, strlen(shaderText), digest);

    /* mangle hash into a shader name */
    sprintf(shader, "%s/%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", mapName.c_str(),
            digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
            digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14], digest[15]);

    /* get shader */
    csi = ShaderInfoForShader(shader);

    /* might be a preexisting shader */
    if (csi->custom) {
        return csi;
    }

    /* clone the existing shader and rename */
    *csi = *si;
    csi->shader = shader;
    csi->custom = true;

    /* store new shader text */
    csi->shaderText = copystring(shaderText);  /* LEAK! */

    /* return it */
    return csi;
}


/*
   EmitVertexRemapShader()
   adds a vertexremapshader key/value pair to worldspawn
*/
void EmitVertexRemapShader(char *from, char *to) {
    byte  digest[16];
    char  key[64], value[256];

    /* dummy check */
    if (strEmptyOrNull(from) || strEmptyOrNull(to)) {
        return;
    }

    /* build value */
    sprintf(value, "%s;%s", from, to);

    /* make md4 hash */
    Com_BlockFullChecksum(value, strlen(value), digest);

    /* make key (this is annoying, as vertexremapshader is precisely 17 characters,
       which is one too long, so we leave off the last byte of the md5 digest) */
    sprintf(key, "vertexremapshader%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
            digest[0], digest[1], digest[2], digest[3], digest[4], digest[5], digest[6], digest[7],
            digest[8], digest[9], digest[10], digest[11], digest[12], digest[13], digest[14]);
    /* no: digest[15] */

    /* add key/value pair to worldspawn */
    entities[0].setKeyValue(key, value);
}


/*
   AllocShaderInfo()
   allocates and initializes a new shader
*/
static shaderInfo_t *AllocShaderInfo() {
    shaderInfo_t *si;

    /* allocate? */
    if (shaderInfo == NULL) {
        shaderInfo = safe_malloc(sizeof(shaderInfo_t) * MAX_SHADER_INFO);
        numShaderInfo = 0;
    }

    /* bounds check */
    if (numShaderInfo == MAX_SHADER_INFO) {
        Error("MAX_SHADER_INFO exceeded. Remove some PK3 files or shader scripts from shaderlist.txt and try again.");
    }

    si = &shaderInfo[numShaderInfo];
    numShaderInfo++;

    /* ydnar: clear to 0 first */
    // memset(si, 0, sizeof(shaderInfo_t));
    new (si) shaderInfo_t{};  // placement new

    /* set defaults */
    ApplyShaderType("default", &si->surfaceFlags, &si->contentFlags, &si->compileFlags);

    si->backsplashFraction = DEF_BACKSPLASH_FRACTION * g_backsplashFractionScale;
    si->backsplashDistance = g_backsplashDistance < -900.0f ? DEF_BACKSPLASH_DISTANCE : g_backsplashDistance;

    si->bounceScale = DEF_RADIOSITY_BOUNCE;

    si->lightStyle = LS_NORMAL;

    si->polygonOffset = false;

    si->shadeAngleDegrees = 0.0f;
    si->lightmapSampleSize = 0;
    si->lightmapSampleOffset = DEFAULT_LIGHTMAP_SAMPLE_OFFSET;
    si->patchShadows = false;
    si->vertexShadows = true;  /* ydnar: changed default behavior */
    si->forceSunlight = false;
    si->lmBrightness = lightmapBrightness;
    si->vertexScale = vertexglobalscale;
    si->notjunc = false;

    /* ydnar: set texture coordinate transform matrix to identity */
    TCModIdentity(si->mod);

    /* ydnar: lightmaps can now be > 128x128 in certain games or an externally generated tga */
    si->lmCustomWidth = lmCustomSizeW;
    si->lmCustomHeight = lmCustomSizeH;

    /* return to sender */
    return si;
}



/*
   FinishShader() - ydnar
   sets a shader's width and height among other things
*/
static void FinishShader(shaderInfo_t *si) {
        int x, y;
        Vector2 st;

        /* don't double-dip */
        if (si->finished) {
            return;
        }

        /* if they're explicitly set, copy from image size */
        if (si->shaderWidth == 0 && si->shaderHeight == 0) {
            si->shaderWidth = si->shaderImage->width;
            si->shaderHeight = si->shaderImage->height;
        }

        /* legacy terrain has explicit image-sized texture projection */
        if (si->legacyTerrain && !si->tcGen) {
            /* set xy texture projection */
            si->tcGen = true;
            si->vecs[0] = { (1.0f / (si->shaderWidth * 0.5f)), 0, 0 };
            si->vecs[1] = { 0, (1.0f / (si->shaderHeight * 0.5f)), 0 };
        }

        /* find pixel coordinates best matching the average color of the image */
        float bestDist = 99999999;
        const Vector2  o (1.0f / si->shaderImage->width, 1.0f / si->shaderImage->height);
        for (y = 0, st[1] = 0.0f; y < si->shaderImage->height; y++, st[1] += o[1]) {
            for (x = 0, st[0] = 0.0f; x < si->shaderImage->width; x++, st[0] += o[0]) {
                /* sample the shader image */
                Color4f color;
                RadSampleImage(si->shaderImage->pixels, si->shaderImage->width, si->shaderImage->height, st, color);

                /* determine error squared */
                const Color4f delta = color - si->averageColor;
                const float dist = vector4_dot(delta, delta);
                if (dist < bestDist) {
                    si->stFlat = st;
                }
            }
        }


        /* set to finished */
        si->finished = true;
}


/*
   LoadShaderImages()
   loads a shader's images
   ydnar: image.c made this a bit simpler
*/
static void LoadShaderImages(shaderInfo_t *si) {
    /* nodraw shaders don't need images */
    if (si->compileFlags & C_NODRAW) {
        si->shaderImage = ImageLoad(DEFAULT_IMAGE);
    } else {
        /* try to load editor image first */
        si->shaderImage = ImageLoad(si->editorImagePath);

        /* then try shadername */
        if (si->shaderImage == NULL) {
            si->shaderImage = ImageLoad(si->shader);
        }

        /* then try implicit image path (note: new behavior!) */
        if (si->shaderImage == NULL) {
            si->shaderImage = ImageLoad(si->implicitImagePath);
        }

        /* then try lightimage (note: new behavior!) */
        if (si->shaderImage == NULL) {
            si->shaderImage = ImageLoad(si->lightImagePath);
        }

        /* otherwise, use default image */
        if (si->shaderImage == NULL) {
            si->shaderImage = ImageLoad(DEFAULT_IMAGE);
            if (g_warnImage && !strEqual(si->shader, "noshader")) {
                Sys_Warning("Couldn't find image for shader %s\n", si->shader.c_str());
            }
        }

        /* load light image */
        si->lightImage = ImageLoad(si->lightImagePath);

        /* load normalmap image (ok if this is NULL) */
        si->normalImage = ImageLoad(si->normalImagePath);
        if (si->normalImage != NULL) {
            Sys_FPrintf(SYS_VRB, "Shader %s has\n"
                                 "    NM %s\n",
                        si->shader.c_str(), si->normalImagePath.c_str());
        }
    }

    /* if no light image, reuse shader image */
    if (si->lightImage == NULL) {
        si->lightImage = si->shaderImage;
    }

    /* create default and average colors */
    const int  count = si->lightImage->width * si->lightImage->height;
    Color4f    color(0, 0, 0, 0);
    for (int i = 0; i < count; i++) {
        color[0] += si->lightImage->pixels[i * 4 + 0];
        color[1] += si->lightImage->pixels[i * 4 + 1];
        color[2] += si->lightImage->pixels[i * 4 + 2];
        color[3] += si->lightImage->pixels[i * 4 + 3];
    }

    if (vector3_length(si->color) == 0.0f) {
        si->color = color.rgb();
        ColorNormalize(si->color);
        si->averageColor = color / count;
    } else {
        si->averageColor.rgb() = si->color;
        si->averageColor.alpha() = 1.0f;
    }
}


/*
   ShaderInfoForShader()
   finds a shaderinfo for a named shader
*/
#define MAX_SHADER_DEPRECATION_DEPTH  16
shaderInfo_t *ShaderInfoForShaderNull(const char *shaderName) {
    if (strEqual(shaderName, "noshader")) {
        return NULL;
    }
    return ShaderInfoForShader(shaderName);
}


shaderInfo_t *ShaderInfoForShader(const char *shaderName) {
    int i;
    int deprecationDepth;
    shaderInfo_t *si;

    /* dummy check */
    if (strEmptyOrNull(shaderName)) {
        Sys_Warning("Null or empty shader name\n");
        shaderName = "missing";
    }

    /* strip off extension */
    auto shader = String512()(PathExtensionless(shaderName));

    /* search for it */
    deprecationDepth = 0;
    for (i = 0; i < numShaderInfo; i++) {
        si = &shaderInfo[i];
        if (striEqual(shader, si->shader)) {
            /* check if shader is deprecated */
            if (deprecationDepth < MAX_SHADER_DEPRECATION_DEPTH && !strEmptyOrNull(si->deprecateShader)) {
                /* override name */
                shader(PathExtensionless(si->deprecateShader));
                /* increase deprecation depth */
                deprecationDepth++;
                if (deprecationDepth == MAX_SHADER_DEPRECATION_DEPTH) {
                    Sys_Warning("Max deprecation depth of %i is reached on shader '%s'\n",
                                MAX_SHADER_DEPRECATION_DEPTH, shader.c_str());
                }
                /* search again from beginning */
                i = -1;
                continue;
            }

            /* load image if necessary */
            if (!si->finished) {
                LoadShaderImages(si);
                FinishShader(si);
            }

            /* return it */
            return si;
        }
    }

    /* allocate a default shader */
    si = AllocShaderInfo();
    si->shader = shader;
    LoadShaderImages(si);
    FinishShader(si);

    /* return it */
    return si;
}



static void Parse1DMatrixAppend(ShaderTextCollector &text, int x, float *m) {
    if (!text.GetToken(true) || !strEqual(token, "(")) {
        Error("Parse1DMatrixAppend(): line %d: '(' not found!\n"
              "File location be: %s\n",  // yarr
              scriptline, g_strLoadedFileLocation);
    }

    for (int i = 0; i < x; i++) {
        if (!text.GetToken(false)) {
            Error("Parse1DMatrixAppend(): line %d: Number not found!\n"
                  "File location be: %s\n",  // yarr
              scriptline, g_strLoadedFileLocation);
        }
        m[i] = atof(token);
    }

    if (!text.GetToken(true) || !strEqual(token, ")")) {
        Error("Parse1DMatrixAppend(): line %d: ')' not found!\n"
              "File location be: %s\n",  // yarr
          scriptline, g_strLoadedFileLocation);
    }
}


/*
   ParseShaderFile()
   parses a shader file into discrete shaderInfo_t
*/
static void ParseShaderFile(const char *filename) {
    ShaderTextCollector  text;

    /* load the shader */
    LoadScriptFile(filename);

    Sys_Printf( "Parsing shader file: \"%s\"\n", filename );

    /* tokenize it */
    while (GetToken(true)) {  /* test for end of file */
        /* shader name is initial token */
        shaderInfo_t *si = AllocShaderInfo();

        si->shader << token;

        /* handle { } section */
        if (!(text.GetToken(true) && strEqual(token, "{"))) {
            Error("ParseShaderFile(): %s, line %d: { not found!\n"
                  "Found instead: %s\n"
                  "Last known shader: %s\n"
                  "File location be: %s\n",  // yarr
                  filename, scriptline, token, si->shader.c_str(), g_strLoadedFileLocation);
        }

        while (text.GetToken(true) && !strEqual(token, "}")) {
            
            /* -----------------------------------------------------------------
               flag and type directives
               ----------------------------------------------------------------- */
            if (striEqual(token, "surfaceparm")) {
                text.GetToken(false);
                Sys_Warning("The \"surfaceparm\" directive is no longer supported!\n");
            } else if (striEqual(token, "$shadertype")) {
                text.GetToken(false);
                if ( !ApplyShaderType(token, &si->surfaceFlags, &si->contentFlags, &si->compileFlags) ) {
                    Sys_Warning( "Unknown shadertype: \"%s\"\n", token );
                }
            } else if (striEqual(token, "$surfaceflag")) {
                text.GetToken(false);
                if ( !ApplyShaderFlag(token, &si->surfaceFlags, nullptr, nullptr) ) {
                    Sys_Warning( "Unknown surfaceflag: \"%s\"\n", token );
                }
            } else if (striEqual(token, "$contentflag")) {
                text.GetToken(false);
                if ( !ApplyShaderFlag(token, nullptr, &si->contentFlags, nullptr) ) {
                    Sys_Warning( "Unknown contentflag: \"%s\"\n", token );
                }
            } else if (striEqual(token, "$compileflag")) {
                text.GetToken(false);
                if ( !ApplyShaderFlag(token, nullptr, nullptr, &si->compileFlags) ) {
                    Sys_Warning( "Unknown compileflag: \"%s\"\n", token );
                }
            /* -----------------------------------------------------------------
               basetexture directives
               ----------------------------------------------------------------- */
            } else if (striEqual(token, "$basetexture")) {
                text.GetToken(false);
                si->editorImagePath(PathExtensionless(token));
            } else if (striEqual(token, "$basetexture2")) {
                text.GetToken(false);
                si->baseTexture2Path(PathExtensionless(token));
            }
            

            /* -----------------------------------------------------------------
               skip
               ----------------------------------------------------------------- */
            /* ignore all other tokens on the line */
            while (TokenAvailable()) {
                text.GetToken(false);
            }
        }

        /* copy shader text to the shaderinfo */
        text.text << '\n';
        si->shaderText = copystring(text.text);
        //% if (vector3_length(si->vecs[0])) {
        //%     Sys_Printf("%s\n", si->shaderText);
        //% }

        /* ydnar: clear shader text buffer */
        text.clear();
    }
}


/*
   ParseCustomInfoParms() - rr2do2
   loads custom info parms file for mods
*/
static void ParseCustomInfoParms() {
    /* file exists? */
    if (vfsGetFileCount("scripts/custinfoparms.txt") == 0) {
        return;
    }

    /* load it */
    LoadScriptFile("scripts/custinfoparms.txt");

    /* clear the array */
    memset(custSurfaceParms, 0, sizeof(custSurfaceParms));
    numCustSurfaceParms = 0;
    bool parsed = false;

    /* parse custom contentflags */
    MatchToken("{");
    while (GetToken(true) && !(parsed = strEqual(token, "}"))) {
        custSurfaceParms[numCustSurfaceParms].name = copystring(token);
        GetToken(false);
        sscanf(token, "%x", &custSurfaceParms[numCustSurfaceParms].contentFlags);
        numCustSurfaceParms++;
    }

    /* any content? */
    if (!parsed) {
        Sys_Warning("Couldn't find valid custom contentsflag section\n");
        return;
    }

    /* parse custom surfaceflags */
    MatchToken("{");
    parsed = false;
    while (GetToken(true) && !(parsed = strEqual(token, "}"))) {
        custSurfaceParms[numCustSurfaceParms].name = copystring(token);
        GetToken(false);
        sscanf(token, "%x", &custSurfaceParms[numCustSurfaceParms].surfaceFlags);
        numCustSurfaceParms++;
    }

    /* any content? */
    if (!parsed) {
        Sys_Warning("Couldn't find valid custom surfaceflag section\n");
    }
}


/*
   LoadShaderInfo()
   the shaders are parsed out of shaderlist.txt from a main directory
   that is, if using -fs_game we ignore the shader scripts that might be in baseq3/
   on linux there's an additional twist, we actually merge the stuff from ~/.q3a/ and from the base dir
*/
void LoadShaderInfo() {
    std::vector<CopiedString> shaderFiles;

    /* rr2do2: parse custom infoparms first */
    if (useCustomInfoParms) {
        ParseCustomInfoParms();
    }

    /* we can pile up several shader files, the one in baseq3 and ones in the mod dir or other spots */
    const auto  filename = StringStream<64>(g_game->shaderPath, "/shaderlist.txt");
    const int   count = vfsGetFileCount(filename);

    /* load them all */
    for (int i = 0; i < count; i++) {
        /* load shader list */
        LoadScriptFile(filename, i);

        /* parse it */
        while (GetToken(true)) {
            /* check for duplicate entries */
            const auto contains = [&shaderFiles](const char *file) {
                for (const CopiedString& str : shaderFiles) {
                    if (striEqual(str.c_str(), file)) {
                        return true;
                    }
                }
                return false;
            };

            if (!path_extension_is(token , "shader")) {
                strcatQ(token, ".shader", std::size(token));
            }

            /* new shader file */
            if (!contains(token)) {
                shaderFiles.emplace_back(token);
            }
        }
    }

    if ( shaderFiles.empty() ) {
        Sys_Printf( "%s", "No shaderlist.txt found: loading all shaders\n" );
        shaderFiles = vfsListShaderFiles( g_game->shaderPath );
    }

    /* parse the shader files */
    for ( const CopiedString& file : shaderFiles ) {
            ParseShaderFile( StringStream<64>( g_game->shaderPath, '/', file ) );
    }

    /* emit some statistics */
    Sys_FPrintf( SYS_VRB, "%9d shaderInfo\n", numShaderInfo );

    if ( numShaderInfo == 0 )
            Sys_FPrintf( SYS_WRN, "WARNING: 0 shaders loaded! Make sure you setup your shader directory and shader definitions properly!\n" );
    
    Sys_FPrintf( SYS_VRB, "\n" );
}
