/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef EXTRACTOR_COMMON_H
#define EXTRACTOR_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include "loadlib.h"
#include <vector>
#include <md5.h>
#include <list>

/// All of the updates for this version of WoW
uint32 const Builds[] = {16016, 16048, 16057, 16309, 16357, 16516, 16650, 16844, 16965, 17116, 17266, 17325, 17345, 17538, 17645, 17688, 17898, 18273};
static uint32 CONF_TargetBuild = 18273; /// Last actual update file under the Data folder

/// Supported locales
static char const* Locales[] = { "enGB", "enUS", "deDE", "esES", "frFR", "koKR", "zhCN", "zhTW", "enCN", "enTW", "esMX", "ruRU", "itIT" };
constexpr auto LOCALES_COUNT = 13; /// Number of supported locales

/// Opens the WoW executable file
FILE* openWoWExe(char const* path = NULL);

/// Gets the build number from the WoW executable
int getBuildNumber(const std::string& path);

/// Gets the core number
int getCoreNumber(const std::string& path);

/// Gets the core number from the build number
int getCoreNumberFromBuild(int iBuildNumber);

/// Gets the list of MPQ files for the specified core number
std::vector<std::string> getMPQListForCore(int coreNumber);

/// Displays a banner with the specified title and core number
void showBanner(const std::string& title, int iCoreNumber);

/// Displays the website banner
void showWebsiteBanner();

/// Checks if the specified map ID is a transport map
bool isTransportMap(int mapID);

/// Checks if the specified map ID should be skipped
bool shouldSkipMap(int mapID, bool m_skipContinents, bool m_skipJunkMaps, bool m_skipBattlegrounds);

/// Type definition for a set of strings
typedef std::set<std::string> StringSet;

/**
 * @brief Test if the specified file exists in the building directory
 *
 * @param file The file to check
 * @return True if the file exists, false otherwise
 */
bool FileExists(const char* file);

/// Creates a directory at the specified path
bool CreateDir(const std::string& sPath);

/// Checks if the specified client file exists
bool ClientFileExists(const char* sFileName);

/// Sets the map magic version based on the core number
std::string setMapMagicVersion(int iCoreNumber);

/// Sets the VMap magic version based on the core number
std::string setVMapMagicVersion(int iCoreNumber);

/**
 * @brief Get "uniform" name for a path (a uniform name has the format <md5hash>-<filename>.<ext>)
 *
 * @param path The path to get the uniform name for
 * @return The uniform name
 */
std::string GetUniformName(std::string& path);

/**
 * @brief Get extension for a file
 *
 * @param file The file to get the extension for
 * @return The extension, if found, or an empty string if not
 */
std::string GetExtension(std::string& file);

/// Scans patches and adds matching archives to the list
bool scan_patches(char* scanmatch, std::vector<std::string>& pArchiveNames);

/// Enumerated core numbers
enum CoreNumber
{
    CLIENT_CLASSIC = 0,
    CLIENT_TBC     = 1,
    CLIENT_WOTLK   = 2,
    CLIENT_CATA    = 3,
    CLIENT_MOP     = 4,
    CLIENT_WOD     = 5,
    CLIENT_LEGION  = 6
};

/// Enumerated model flags
enum ModelFlags
{
    MOD_M2 = 1,
    MOD_WORLDSPAWN = 1 << 1,
    MOD_HAS_BOUND = 1 << 2
};

/**
 * @brief Struct representing a 3D vector
 */
typedef struct
{
    float x; /**< X coordinate */
    float y; /**< Y coordinate */
    float z; /**< Z coordinate */
} svec;

/**
 * @brief Struct representing a cell
 */
typedef struct
{
    float v9[16 * 8 + 1][16 * 8 + 1]; /**< Array of v9 values */
    float v8[16 * 8][16 * 8]; /**< Array of v8 values */
} Cell;

/**
 * @brief Struct representing a chunk
 */
typedef struct
{
    double v9[9][9]; /**< Array of v9 values */
    double v8[8][8]; /**< Array of v8 values */
    uint16 area_id; /**< Area ID */
    float waterlevel[9][9]; /**< Water level values */
    uint8 flag; /**< Flag */
} chunk;

/**
 * @brief Struct representing a map cell
 */
typedef struct
{
    chunk ch[16][16]; /**< Array of chunks */
} mcell;

/**
 * @brief Struct representing the header of a map chunk
 */
struct MapChunkHeader
{
    uint32 flags; /**< Flags */
    uint32 ix; /**< X index */
    uint32 iy; /**< Y index */
    uint32 nLayers; /**< Number of layers */
    uint32 nDoodadRefs; /**< Number of doodad references */
    uint32 ofsHeight; /**< Offset to height data */
    uint32 ofsNormal; /**< Offset to normal data */
    uint32 ofsLayer; /**< Offset to layer data */
    uint32 ofsRefs; /**< Offset to references */
    uint32 ofsAlpha; /**< Offset to alpha data */
    uint32 sizeAlpha; /**< Size of alpha data */
    uint32 ofsShadow; /**< Offset to shadow data */
    uint32 sizeShadow; /**< Size of shadow data */
    uint32 areaid; /**< Area ID */
    uint32 nMapObjRefs; /**< Number of map object references */
    uint32 holes; /**< Holes */
    uint16 s1; /**< Unknown */
    uint16 s2; /**< Unknown */
    uint32 d1; /**< Unknown */
    uint32 d2; /**< Unknown */
    uint32 d3; /**< Unknown */
    uint32 predTex; /**< Predicted texture */
    uint32 nEffectDoodad; /**< Number of effect doodads */
    uint32 ofsSndEmitters; /**< Offset to sound emitters */
    uint32 nSndEmitters; /**< Number of sound emitters */
    uint32 ofsLiquid; /**< Offset to liquid data */
    uint32 sizeLiquid; /**< Size of liquid data */
    float  zpos; /**< Z position */
    float  xpos; /**< X position */
    float  ypos; /**< Y position */
    uint32 textureId; /**< Texture ID */
    uint32 props; /**< Properties */
    uint32 effectId; /**< Effect ID */
};

/**
 * @brief Struct representing a 3D vector with double precision
 */
struct vec
{
    double x; /**< X coordinate */
    double y; /**< Y coordinate */
    double z; /**< Z coordinate */
};

/**
 * @brief Struct representing a triangle
 */
struct triangle
{
    vec v[3]; /**< Array of vertices */
};

#endif // EXTRACTOR_COMMON_H

