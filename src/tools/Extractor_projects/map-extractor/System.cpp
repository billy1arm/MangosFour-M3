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

#include <stdio.h>
#include <set>
#include "dbcfile.h"
#include <mpq.h>
#include <adt.h>
#include <wdt.h>
#include "ExtractorCommon.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <errno.h>
#include <regex>

// From Extractor
#include "adtfile.h"
#include "wdtfile.h"
#include "wmo.h"
#include "Auth/md5.h"

#ifndef WIN32
#include <unistd.h>
/* This isn't the nicest way to do things..
 * TODO: Fix this with snprintf instead and check that it still works
 */
#define sprintf_s sprintf
#endif
#include <cstdlib>

#ifdef WIN32
#include "direct.h"
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#include <limits>

extern ArchiveSet gOpenArchives;    /**< maintains a list of all currently opened MPQ archives. */

/**
 * @brief The dataFile type is a structure that encapsulates information about a file, including its identifiers, names, handle, and folder organization attributes.
 *
 */
typedef struct
{
    int lookupId = 0;
    string fileName;
    string displayName;
    HANDLE fileHandle = nullptr;
    string parentFilename;             /**< The filename of the parent File */
    bool inSubfolder = false;          /**< Indicates whether its in subfolder */
    string subfolderPath;              /**< The subfolder path */
    uint16 uint16Value = 0;
    int mpqId = 0;
} dataFile;

std::string output_path = ".";        /**< Output path for extracted files */
std::string input_path = ".";         /**< Input path for game client archives */
uint32 maxAreaId = 0;                 /**< Maximum area ID */
int iCoreNumber = 0;                  /**< Core number of the client */
int iBuildNumber = 0;                 /**< Build number of the client */
bool debugLog = false;                /**< Debug log flag */
std::vector<dataFile> MapList;        /**< List of map files */
std::vector<dataFile> AreaList;       /**< List of area files */
std::vector<dataFile> LiquidList;     /**< List of liquid files */
std::vector<std::string> LiquidTypeList; /**< List of liquid types */
std::string szWorkDirWmo   = "/Buildings"; /**< Working directory for WMO files */
std::string szRawVMAPMagic = "VMAP000"; /**< VMAP magic string */
std::vector<dataFile> DBCFiles;       /**< List of DBC files */
std::vector<dataFile> DB2Files;       /**< List of DB2 files */
std::vector<dataFile> WDTFiles;       /**< List of WDT files */
std::vector<dataFile> ADTFiles;       /**< List of ADT files */
std::vector<dataFile> FinalMPQList;   /**< Final list of MPQ files */
std::string outDir = std::string(output_path) + "/vmaps"; /**< Output directory for VMAP files */

/**
 * @brief Data types which can be extracted
 *
 */
enum Extract
{
    EXTRACT_MAP = 1,
    EXTRACT_DBC_EXTRACTORS_ONLY = 2,
    EXTRACT_DBC_EXTRACTORS_AND_CORE = 4,
    EXTRACT_DBC_EXTRACTORS_ALL = 8,
};

int   CONF_extract = EXTRACT_MAP | EXTRACT_DBC_EXTRACTORS_ALL; /**< Select data for extract */
bool  CONF_allow_height_limit       = true;     /**< Allows to limit minimum height */
float CONF_use_minHeight            = -500.0f;  /**< Default minimum height */
float CONF_flat_height_delta_limit = 0.005f;    /**< If max - min less this value - surface is flat */
float CONF_flat_liquid_delta_limit = 0.001f;    /**< If max - min less this value - liquid surface is flat */

#if defined (CLASSIC) || defined (TBC)
int MAP_LIQUID_TYPE_NO_WATER = 0x00;
int MAP_LIQUID_TYPE_MAGMA    = 0x01;
int MAP_LIQUID_TYPE_OCEAN    = 0x02;
int MAP_LIQUID_TYPE_SLIME    = 0x04;
int MAP_LIQUID_TYPE_WATER    = 0x08;
#else
int MAP_LIQUID_TYPE_NO_WATER = 0x00;
int MAP_LIQUID_TYPE_WATER    = 0x01;
int MAP_LIQUID_TYPE_OCEAN    = 0x02;
int MAP_LIQUID_TYPE_MAGMA    = 0x04;
int MAP_LIQUID_TYPE_SLIME    = 0x08;
#endif

static int LANG_COUNT = 13;

// Map file format data
std::string MAP_MAGIC           = "MAPS"; /**< Magic identifier for map files */
std::string MAP_VERSION_MAGIC   = "0000"; /**< Version identifier for map files */
std::string MAP_AREA_MAGIC      = "AREA"; /**< Magic identifier for area map */
std::string MAP_HEIGHT_MAGIC    = "MHGT"; /**< Magic identifier for height map */
std::string MAP_LIQUID_MAGIC    = "MLIQ"; /**< Magic identifier for liquid map */

/**
 * @brief Structure representing the header of a map file.
 *
 */
struct map_fileheader
{
    std::string mapMagic = "";     /**< Magic identifier for the map file format */
    std::string versionMagic = ""; /**< Version identifier for the map file format */
    uint32 buildMagic = 0;         /**< Build number of the client */
    uint32 areaMapOffset = 0;      /**< Offset to the area map data */
    uint32 areaMapSize = 0;        /**< Size of the area map data */
    uint32 heightMapOffset = 0;    /**< Offset to the height map data */
    uint32 heightMapSize = 0;      /**< Size of the height map data */
    uint32 liquidMapOffset = 0;    /**< Offset to the liquid map data */
    uint32 liquidMapSize = 0;      /**< Size of the liquid map data */
    uint32 holesOffset = 0;        /**< Offset to the holes data */
    uint32 holesSize = 0;          /**< Size of the holes data */
};

#define MAP_AREA_NO_AREA      0x0001

/**
 * @brief Structure representing the header of an area map.
 *
 */
struct map_areaHeader
{
    std::string fourcc; /**< Four-character code (magic number) identifying the area map format */
    uint16 flags = 0;   /**< Flags indicating properties of the area map */
    uint16 gridArea = 0; /**< Grid area identifier */
};

#define MAP_HEIGHT_NO_HEIGHT  0x0001
#define MAP_HEIGHT_AS_INT16   0x0002
#define MAP_HEIGHT_AS_INT8    0x0004

/**
 * @brief Structure representing the header of a height map.
 *
 */
struct map_heightHeader
{
    std::string fourcc;     /**< Four-character code (magic number) identifying the height map format */
    uint32 flags;           /**< Flags indicating properties of the height map */
    float gridHeight;       /**< Minimum height value in the grid */
    float gridMaxHeight;    /**< Maximum height value in the grid */
};

#define MAP_LIQUID_TYPE_DARK_WATER  0x10
#define MAP_LIQUID_TYPE_WMO_WATER   0x20

#define MAP_LIQUID_NO_TYPE    0x0001
#define MAP_LIQUID_NO_HEIGHT  0x0002

/**
 * @brief Structure representing the header of a liquid map.
 *
 */
struct map_liquidHeader
{
    std::string fourcc = "";          /**< Four-character code (magic number) identifying the liquid map format */
    uint16 flags = 0;                 /**< Flags indicating properties of the liquid map */
    uint16 liquidType = 0;            /**< Type of liquid */
    uint8  offsetX = 0;               /**< X offset */
    uint8  offsetY = 0;               /**< Y offset */
    uint8  width = 0;                 /**< Width of the liquid map */
    uint8  height = 0;                /**< Height of the liquid map */
    float  liquidLevel = 0.0f;        /**< Liquid level */
};

uint16 area_flags[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];      /**< Temporary grid data store */

float V8[ADT_GRID_SIZE][ADT_GRID_SIZE];                         /**< Height values for V8 grid */
float V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];                 /**< Height values for V9 grid */
uint16 uint16_V8[ADT_GRID_SIZE][ADT_GRID_SIZE];                 /**< Height values for V8 grid as uint16 */
uint16 uint16_V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];         /**< Height values for V9 grid as uint16 */
uint8  uint8_V8[ADT_GRID_SIZE][ADT_GRID_SIZE];                  /**< Height values for V8 grid as uint8 */
uint8  uint8_V9[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];          /**< Height values for V9 grid as uint8 */

uint16 liquid_entry[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];    /**< Liquid entry data */
uint8 liquid_flags[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];     /**< Liquid flags */
bool  liquid_show[ADT_GRID_SIZE][ADT_GRID_SIZE];                /**< Liquid show flags */
float liquid_height[ADT_GRID_SIZE + 1][ADT_GRID_SIZE + 1];      /**< Liquid height values */

/**
 * @brief Extracts a file from an MPQ archive.
 *
 * @param dbcFiles Vector of dataFile structures representing the files to be extracted.
 * @param mpqPath Path within the MPQ archive.
 * @param fileMask File mask to filter files.
 * @param localPath Local directory to save the extracted files.
 * @param trimLength Flag to indicate if the path should be trimmed.
 * @return int Number of files extracted.
 */
int ExtractFilefromMPQ(std::vector<dataFile>& dbcFiles, const char * mpqPath,string fileMask,string localPath, bool trimLength);
bool ParseMapFiles(uint32 mapIndex, uint32 xcoord, uint32 ycoord, std::string adt_filename);
/**
 * @brief Appends a list of files to a vector based on a file mask.
 *
 * @param filelist Vector to append the file list to.
 * @param fileMask File mask to filter files.
 */
void AppendFileListTo(std::vector<dataFile>& fileList, const char* fileMask);

/**
 * @brief Reads DBC files from an MPQ archive.
 *
 * @param fileName Name of the DBC file to read.
 * @param mapList Vector to store the read data.
 * @param dbcType Type of DBC file.
 */
void NewReadDbcFromMPQ(const char* fileName, std::vector<dataFile>& mapList, int dbcType);

/**
 * @brief Extracts WDT files from an MPQ archive.
 *
 * @param mpqFilePath Path within the MPQ archive.
 * @param localPath Local directory to save the extracted files.
 * @return int Number of files extracted.
 */
int ExtractWDTFilefromMPQ(string mpqFilePath, string localPath);

/**
 * @brief Extracts ADT files from an MPQ archive.
 *
 * @param mpqFilePath Path within the MPQ archive.
 * @param localPath Local directory to save the extracted files.
 * @return int Number of files extracted.
 */
int ExtractADTFilesfromMPQ(string mpqFilePath, string localPath);

/**
 * @brief Returns the area list ID for a given lookup ID.
 *
 * @param lookupId Lookup ID.
 * @return int Area list ID.
 */
int ReturnAreaListId(int lookupId);

// VMAP Additions

/**
 * @brief Displays usage information for the program.
 *
 * @param prg Program name.
 */
static void Usage(char* prg)
{
    printf(" Usage: %s [OPTION]\n\n", prg);
    printf(" Extract client database files and generate map files.\n");
    printf("   -h, --help            show the usage\n");
    printf("   -i, --input <path>    search path for game client archives\n");
    printf("   -o, --output <path>   target path for generated files\n");
    printf("   -e, --extract #       extract specified client data. 1 = maps, 2 = DBCs,\n");
    printf("                         3 = both. Defaults to extracting both.\n");
    printf("\n");
    printf(" Example:\n");
    printf(" - use input path and do not flatten maps:\n");
    printf("   %s -f 0 -i \"c:\\games\\world of warcraft\"\n", prg);
    exit(1);
}

/**
 * @brief Handles command-line arguments.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void HandleArgs(int argc, char* arg[])
{
    for (int c = 1; c < argc; ++c)
    {
        // i - input path
        // o - output path
        // e - extract only MAP(1)/DBC(2) - standard both(3)
        // f - use float to int conversion
        // h - limit minimum height
        if (arg[c][0] != '-')
        {
            Usage(arg[0]);
        }

        switch (arg[c][1])
        {
            case 'i':
                if (c + 1 < argc)                           // all ok
                {
                    input_path = arg[(c++) + 1];
                }
                else
                {
                    Usage(arg[0]);
                }
                break;
            case 'o':
                if (c + 1 < argc)                           // all ok
                {
                    output_path = arg[(c++) + 1];
                }
                else
                {
                    Usage(arg[0]);
                }
                break;
            case 'e':
                if (c + 1 < argc)                           // all ok
                {
                    CONF_extract = atoi(arg[(c++) + 1]);
                    if (!(CONF_extract > 0 && CONF_extract < 4))
                    {
                        Usage(arg[0]);
                    }
                }
                else
                {
                    Usage(arg[0]);
                }
                break;
            default:
                Usage(arg[0]);
                break;
        }
    }
}

/**
 * @brief Returns the area list ID for a given lookup ID.
 *
 * @param lookupId Lookup ID.
 * @return int Area list ID.
 */
int ReturnAreaListId(int lookupId)
{
    if (lookupId <= AreaList.size())
    {
        for (int i = 0; i < AreaList.size(); ++i)
        {
            if (AreaList[i].lookupId == lookupId)
            {
                return i;
            }
        }
    }
    return 0;
}

/**
 * @brief Converts an ADT file to a map file.
 *
 * @param adt_filename The ADT filename to be loaded.
 * @param output_filename The location where the .map file is saved.
 * @return bool True if the conversion is successful, false otherwise.
 */
static bool ConvertADT(char* adt_filename, char* output_filename)
{
    ADT_file adt;

    if (!adt.loadFileFromDisk(adt_filename,false))
    {
        return false;
    }

    // TODO: Not sure why this fails
    //adt_MCIN* cells = adt.a_grid->getMCIN();
    //if (!cells)
    //{
    //    //printf("Can not find cells in '%s'\n", filename);
    //    return false;
    //}

    std::string path = output_path;
    path += "/maps/";
    CreateDir(path);

    memset(liquid_show, 0, sizeof(liquid_show));
    memset(liquid_flags, 0, sizeof(liquid_flags));
    memset(liquid_entry, 0, sizeof(liquid_entry));

    // Prepare map header
    map_fileheader map;
    map.mapMagic = MAP_MAGIC;
    map.versionMagic =MAP_VERSION_MAGIC;
    map.buildMagic = iBuildNumber;

    // Get area flags data
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
            //adt_MCNK* cell = cells->getMCNK(i, j);
            adt_MCNK* cell = adt.cells[i][j];
            uint32 areaid = cell->areaid;
            if (areaid && areaid <= maxAreaId)
            {
                if (AreaList[ReturnAreaListId(areaid)].uint16Value != 0xffff)
                {
                    area_flags[i][j] = AreaList[ReturnAreaListId(areaid)].uint16Value;
                    continue;
                }
                if (debugLog)
                {
                    printf("    WARNING: File: %s - Can not find area flag for area %u [%d, %d].\n", adt_filename, areaid, cell->ix, cell->iy);
                }
            }
            area_flags[i][j] = 0xffff;
        }
    }

    //============================================
    // Try pack area data
    //============================================
    bool fullAreaData = false;
    uint32 areaflag = area_flags[0][0];
    for (int y = 0; y < ADT_CELLS_PER_GRID; y++)
    {
        for (int x = 0; x < ADT_CELLS_PER_GRID; x++)
        {
            if (area_flags[y][x] != areaflag)
            {
                fullAreaData = true;
                break;
            }
        }
    }

    map.areaMapOffset = sizeof(map);
    map.areaMapSize   = sizeof(map_areaHeader);

    map_areaHeader areaHeader;
    areaHeader.fourcc = MAP_AREA_MAGIC;
    areaHeader.flags = 0;
    if (fullAreaData)
    {
        areaHeader.gridArea = 0;
        map.areaMapSize += sizeof(area_flags);
    }
    else
    {
        areaHeader.flags |= MAP_AREA_NO_AREA;
        areaHeader.gridArea = (uint16)areaflag;
    }

    //
    // Get Height map from grid
    //
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
//            adt_MCNK* cell = cells->getMCNK(i, j);
            adt_MCNK* cell = adt.cells[i][j];
            if (!cell)
            {
                continue;
            }
            // Height values for triangles stored in order:
            // 1     2     3     4     5     6     7     8     9
            //    10    11    12    13    14    15    16    17
            // 18    19    20    21    22    23    24    25    26
            //    27    28    29    30    31    32    33    34
            // . . . . . . . .
            // For better get height values merge it to V9 and V8 map
            // V9 height map:
            // 1     2     3     4     5     6     7     8     9
            // 18    19    20    21    22    23    24    25    26
            // . . . . . . . .
            // V8 height map:
            //    10    11    12    13    14    15    16    17
            //    27    28    29    30    31    32    33    34
            // . . . . . . . .

            // Set map height as grid height
            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V9[cy][cx] = cell->ypos;
                }
            }
            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V8[cy][cx] = cell->ypos;
                }
            }
            // Get custom height
            adt_MCVT* v = cell->getMCVT();
            if (!v)
            {
                continue;
            }
            // get V9 height map
            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V9[cy][cx] += v->height_map[y * (ADT_CELL_SIZE * 2 + 1) + x];
                }
            }
            // get V8 height map
            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    V8[cy][cx] += v->height_map[y * (ADT_CELL_SIZE * 2 + 1) + ADT_CELL_SIZE + 1 + x];
                }
            }
        }
    }
    //============================================
    // Try pack height data
    //============================================
    float maxHeight = -20000;
    float minHeight =  20000;
    for (int y = 0; y < ADT_GRID_SIZE; y++)
    {
        for (int x = 0; x < ADT_GRID_SIZE; x++)
        {
            float h = V8[y][x];
            if (maxHeight < h)
            {
                maxHeight = h;
            }
            if (minHeight > h)
            {
                minHeight = h;
            }
        }
    }
    for (int y = 0; y <= ADT_GRID_SIZE; y++)
    {
        for (int x = 0; x <= ADT_GRID_SIZE; x++)
        {
            float h = V9[y][x];
            if (maxHeight < h)
            {
                maxHeight = h;
            }
            if (minHeight > h)
            {
                minHeight = h;
            }
        }
    }

    // Check for allow limit minimum height (not store height in deep ocean - allow save some memory)
    if (CONF_allow_height_limit && minHeight < CONF_use_minHeight)
    {
        for (int y = 0; y < ADT_GRID_SIZE; y++)
        {
            for (int x = 0; x < ADT_GRID_SIZE; x++)
            {
                if (V8[y][x] < CONF_use_minHeight)
                {
                    V8[y][x] = CONF_use_minHeight;
                }
            }
        }
        for (int y = 0; y <= ADT_GRID_SIZE; y++)
        {
            for (int x = 0; x <= ADT_GRID_SIZE; x++)
            {
                if (V9[y][x] < CONF_use_minHeight)
                {
                    V9[y][x] = CONF_use_minHeight;
                }
            }
        }
        if (minHeight < CONF_use_minHeight)
        {
            minHeight = CONF_use_minHeight;
        }
        if (maxHeight < CONF_use_minHeight)
        {
            maxHeight = CONF_use_minHeight;
        }
    }

    map.heightMapOffset = map.areaMapOffset + map.areaMapSize;
    map.heightMapSize = sizeof(map_heightHeader);

    map_heightHeader heightHeader{};
    heightHeader.fourcc = MAP_HEIGHT_MAGIC;
    heightHeader.flags = 0;
    heightHeader.gridHeight    = minHeight;
    heightHeader.gridMaxHeight = maxHeight;

    if (maxHeight == minHeight)
    {
        heightHeader.flags |= MAP_HEIGHT_NO_HEIGHT;
    }

    // Not need store if flat surface
    if ((maxHeight - minHeight) < CONF_flat_height_delta_limit)
    {
        heightHeader.flags |= MAP_HEIGHT_NO_HEIGHT;
    }

    // Try store as packed in uint16 or uint8 values
    if (!(heightHeader.flags & MAP_HEIGHT_NO_HEIGHT))
    {
        float step = 0.0f;
        //// Try Store as uint values
        //if (CONF_allow_float_to_int)
        //{
        //    float diff = maxHeight - minHeight;

        //    heightHeader.flags |= MAP_HEIGHT_AS_INT16;
        //    step = selectUInt16StepStore(diff);
        //}

        // Pack it to int values if need
        if (heightHeader.flags & MAP_HEIGHT_AS_INT8)
        {
            for (int y = 0; y < ADT_GRID_SIZE; y++)
            {
                for (int x = 0; x < ADT_GRID_SIZE; x++)
                {
                    uint8_V8[y][x] = uint8((V8[y][x] - minHeight) * step + 0.5f);
                }
            }
            for (int y = 0; y <= ADT_GRID_SIZE; y++)
            {
                for (int x = 0; x <= ADT_GRID_SIZE; x++)
                {
                    uint8_V9[y][x] = uint8((V9[y][x] - minHeight) * step + 0.5f);
                }
            }
            map.heightMapSize += sizeof(uint8_V9) + sizeof(uint8_V8);
        }
        else if (heightHeader.flags & MAP_HEIGHT_AS_INT16)
        {
            for (int y = 0; y < ADT_GRID_SIZE; y++)
            {
                for (int x = 0; x < ADT_GRID_SIZE; x++)
                {
                    uint16_V8[y][x] = uint16((V8[y][x] - minHeight) * step + 0.5f);
                }
            }
            for (int y = 0; y <= ADT_GRID_SIZE; y++)
            {
                for (int x = 0; x <= ADT_GRID_SIZE; x++)
                {
                    uint16_V9[y][x] = uint16((V9[y][x] - minHeight) * step + 0.5f);
                }
            }
            map.heightMapSize += sizeof(uint16_V9) + sizeof(uint16_V8);
        }
        else
        {
            map.heightMapSize += sizeof(V9) + sizeof(V8);
        }
    }

    // Get from MCLQ chunk (old)
    for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
        {
            //adt_MCNK* cell = cells->getMCNK(i, j);
            adt_MCNK* cell = adt.cells[i][j];

            if (!cell)
            {
                continue;
            }

            adt_MCLQ* liquid = cell->getMCLQ();
            int count = 0;
            if (!liquid || cell->sizeMCLQ <= 8)
            {
                continue;
            }

            for (int y = 0; y < ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x < ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    if (liquid->flags[y][x] != 0x0F)
                    {
                        liquid_show[cy][cx] = true;
                        if (liquid->flags[y][x] & (1 << 7))
                        {
                            liquid_flags[i][j] |= MAP_LIQUID_TYPE_DARK_WATER;
                        }
                        ++count;
                    }
                }
            }

            uint32 c_flag = cell->flags;
            if (c_flag & (1 << 2))
            {
                liquid_entry[i][j] = 1;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_WATER;            // water
            }
            if (c_flag & (1 << 3))
            {
                liquid_entry[i][j] = 2;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_OCEAN;            // ocean
            }
            if (c_flag & (1 << 4))
            {
                liquid_entry[i][j] = 3;
                liquid_flags[i][j] |= MAP_LIQUID_TYPE_MAGMA;            // magma/slime
            }

            if (!count && liquid_flags[i][j])
            {
                fprintf(stderr, "Wrong liquid type detected in MCLQ chunk");
            }

            for (int y = 0; y <= ADT_CELL_SIZE; y++)
            {
                int cy = i * ADT_CELL_SIZE + y;
                for (int x = 0; x <= ADT_CELL_SIZE; x++)
                {
                    int cx = j * ADT_CELL_SIZE + x;
                    liquid_height[cy][cx] = liquid->liquid[y][x].height;
                }
            }
        }
    }

    // Get liquid map for grid (in WOTLK used MH2O chunk)
    adt_MH2O* h2o = adt.a_grid->getMH2O();
    if (h2o)
    {
        for (int i = 0; i < ADT_CELLS_PER_GRID; i++)
        {
            for (int j = 0; j < ADT_CELLS_PER_GRID; j++)
            {
                adt_liquid_header* h = h2o->getLiquidData(i, j);
                if (!h)
                {
                    continue;
                }

                int count = 0;
                uint64 show = h2o->getLiquidShowMap(h);
                for (int y = 0; y < h->height; y++)
                {
                    int cy = i * ADT_CELL_SIZE + y + h->yOffset;
                    for (int x = 0; x < h->width; x++)
                    {
                        int cx = j * ADT_CELL_SIZE + x + h->xOffset;
                        if (show & 1)
                        {
                            liquid_show[cy][cx] = true;
                            ++count;
                        }
                        show >>= 1;
                    }
                }

                liquid_entry[i][j] = h->liquidType;
                switch (LiquidList[h->liquidType].uint16Value)
                {
                    case LIQUID_TYPE_WATER:
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_WATER;
                        break;
                    case LIQUID_TYPE_OCEAN:
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_OCEAN;
                        break;
                    case LIQUID_TYPE_MAGMA:
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_MAGMA;
                        break;
                    case LIQUID_TYPE_SLIME:
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_SLIME;
                        break;
                    default:
                        if (debugLog)
                        {
                            printf("   WARNING:Can not find liquid type %u for map %s - Chunk: %d,%d\n", h->liquidType, adt_filename, i, j);
                        }
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_NO_WATER;
                        break;
                }
                // Dark water detect
                if (LiquidList[h->liquidType].uint16Value == LIQUID_TYPE_OCEAN)
                {
                    uint8* lm = h2o->getLiquidLightMap(h);
                    if (!lm)
                    {
                        liquid_flags[i][j] |= MAP_LIQUID_TYPE_DARK_WATER;
                    }
                }

                if (!count && liquid_flags[i][j])
                {
                    printf("Wrong liquid type detected in MH2O chunk");
                }

                float* height = h2o->getLiquidHeightMap(h);
                int pos = 0;
                for (int y = 0; y <= h->height; y++)
                {
                    int cy = i * ADT_CELL_SIZE + y + h->yOffset;
                    for (int x = 0; x <= h->width; x++)
                    {
                        int cx = j * ADT_CELL_SIZE + x + h->xOffset;
                        if (height)
                        {
                            liquid_height[cy][cx] = height[pos];
                        }
                        else
                        {
                            liquid_height[cy][cx] = h->heightLevel1;
                        }
                        pos++;
                    }
                }
            }
        }
    }
    //============================================
    // Pack liquid data
    //============================================
    uint8 type = liquid_flags[0][0];
    bool fullType = false;
    for (int y = 0; y < ADT_CELLS_PER_GRID; y++)
    {
        for (int x = 0; x < ADT_CELLS_PER_GRID; x++)
        {
            if (liquid_flags[y][x] != type)
            {
                fullType = true;
                y = ADT_CELLS_PER_GRID;
                break;
            }
        }
    }

    map_liquidHeader liquidHeader;

    // no water data (if all grid have 0 liquid type)
    if (type == 0 && !fullType)
    {
        // No liquid data
        map.liquidMapOffset = 0;
        map.liquidMapSize   = 0;
    }
    else
    {
        int minX = 255, minY = 255;
        int maxX = 0, maxY = 0;
        maxHeight = -20000;
        minHeight = 20000;
        for (int y = 0; y < ADT_GRID_SIZE; y++)
        {
            for (int x = 0; x < ADT_GRID_SIZE; x++)
            {
                if (liquid_show[y][x])
                {
                    if (minX > x)
                    {
                        minX = x;
                    }
                    if (maxX < x)
                    {
                        maxX = x;
                    }
                    if (minY > y)
                    {
                        minY = y;
                    }
                    if (maxY < y)
                    {
                        maxY = y;
                    }
                    float h = liquid_height[y][x];
                    if (maxHeight < h)
                    {
                        maxHeight = h;
                    }
                    if (minHeight > h)
                    {
                        minHeight = h;
                    }
                }
                else
                {
                    liquid_height[y][x] = CONF_use_minHeight;
                }
            }
        }
        map.liquidMapOffset = map.heightMapOffset + map.heightMapSize;
        map.liquidMapSize = sizeof(map_liquidHeader);
        liquidHeader.fourcc = MAP_LIQUID_MAGIC;
        liquidHeader.flags = 0;
        liquidHeader.liquidType = 0;
        liquidHeader.offsetX = minX;
        liquidHeader.offsetY = minY;
        liquidHeader.width   = maxX - minX + 1 + 1;
        liquidHeader.height  = maxY - minY + 1 + 1;
        liquidHeader.liquidLevel = minHeight;

        if (maxHeight == minHeight)
        {
            liquidHeader.flags |= MAP_LIQUID_NO_HEIGHT;
        }

        // Not need store if flat surface
        if ((maxHeight - minHeight) < CONF_flat_liquid_delta_limit)
        {
            liquidHeader.flags |= MAP_LIQUID_NO_HEIGHT;
        }

        if (!fullType)
        {
            liquidHeader.flags |= MAP_LIQUID_NO_TYPE;
        }

        if (liquidHeader.flags & MAP_LIQUID_NO_TYPE)
        {
            liquidHeader.liquidType = type;
        }
        else
        {
            map.liquidMapSize += sizeof(liquid_entry) + sizeof(liquid_flags);
        }

        if (!(liquidHeader.flags & MAP_LIQUID_NO_HEIGHT))
        {
            map.liquidMapSize += sizeof(float) * liquidHeader.width * liquidHeader.height;
        }
    }

    // map hole info
    uint16 holes[ADT_CELLS_PER_GRID][ADT_CELLS_PER_GRID];

    if (map.liquidMapOffset)
    {
        map.holesOffset = map.liquidMapOffset + map.liquidMapSize;
    }
    else
    {
        map.holesOffset = map.heightMapOffset + map.heightMapSize;
    }

    map.holesSize = sizeof(holes);
    memset(holes, 0, map.holesSize);

    for (int i = 0; i < ADT_CELLS_PER_GRID; ++i)
    {
        for (int j = 0; j < ADT_CELLS_PER_GRID; ++j)
        {
            //adt_MCNK* cell = cells->getMCNK(i, j);
            adt_MCNK* cell = adt.cells[i][j];

            if (!cell)
            {
                continue;
            }
            holes[i][j] = cell->holes;
        }
    }

    // Ok all data prepared - store it
    FILE* output = fopen(output_filename, "wb");
    if (!output)
    {
        printf("Can not create the output file '%s'\n", output_filename);
        return false;
    }
    fwrite(&map, sizeof(map), 1, output);

    // Store area data
    fwrite(&areaHeader, sizeof(areaHeader), 1, output);
    if (!(areaHeader.flags & MAP_AREA_NO_AREA))
    {
        fwrite(area_flags, sizeof(area_flags), 1, output);
    }

    // Store height data
    fwrite(&heightHeader, sizeof(heightHeader), 1, output);
    if (!(heightHeader.flags & MAP_HEIGHT_NO_HEIGHT))
    {
        if (heightHeader.flags & MAP_HEIGHT_AS_INT16)
        {
            fwrite(uint16_V9, sizeof(uint16_V9), 1, output);
            fwrite(uint16_V8, sizeof(uint16_V8), 1, output);
        }
        else if (heightHeader.flags & MAP_HEIGHT_AS_INT8)
        {
            fwrite(uint8_V9, sizeof(uint8_V9), 1, output);
            fwrite(uint8_V8, sizeof(uint8_V8), 1, output);
        }
        else
        {
            fwrite(V9, sizeof(V9), 1, output);
            fwrite(V8, sizeof(V8), 1, output);
        }
    }

    // Store liquid data if need
    if (map.liquidMapOffset)
    {
        fwrite(&liquidHeader, sizeof(liquidHeader), 1, output);
        if (!(liquidHeader.flags & MAP_LIQUID_NO_TYPE))
        {
            fwrite(liquid_entry, sizeof(liquid_entry), 1, output);
            fwrite(liquid_flags, sizeof(liquid_flags), 1, output);
        }
        if (!(liquidHeader.flags & MAP_LIQUID_NO_HEIGHT))
        {
            for (int y = 0; y < liquidHeader.height; y++)
            {
                fwrite(&liquid_height[y + liquidHeader.offsetY][liquidHeader.offsetX], sizeof(float), liquidHeader.width, output);
            }
        }
    }

    // store hole data
    fwrite(holes, map.holesSize, 1, output);

    fclose(output);

    return true;
}

//void AppendPatchMPQFilesToList(char const* subdir, char const* suffix, char const* section, Updates& updates)
//{
//    char dirname[2048];
//    if (subdir)
//    {
//        sprintf(dirname, "%s/Data/%s", input_path, subdir);
//    }
//    else
//    {
//        sprintf(dirname, "%s/Data", input_path);
//    }
//
//    char scanname[512];
//    if (suffix)
//    {
//        sprintf(scanname, "wow-update-%s-%%u.MPQ", suffix);
//    }
//    else
//    {
//        sprintf(scanname, "wow-update-%%u.MPQ");
//    }
//
//#ifdef WIN32
//
//    char maskname[512];
//    if (suffix)
//    {
//        sprintf(maskname, "%s/wow-update-%s-*.MPQ", dirname, suffix);
//    }
//    else
//    {
//        sprintf(maskname, "%s/wow-update-*.MPQ", dirname);
//    }
//
//    WIN32_FIND_DATA ffd;
//    HANDLE hFind = FindFirstFile(maskname, &ffd);
//
//    if (hFind != INVALID_HANDLE_VALUE)
//    {
//        do
//        {
//            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
//            {
//                continue;
//            }
//
//            uint32 ubuild = 0;
//            if (sscanf(ffd.cFileName, scanname, &ubuild) == 1 && (!iCoreNumber || ubuild <= iCoreNumber))
//            {
//                updates[ubuild] = UpdatesPair(ffd.cFileName, section);
//            }
//        }
//        while (FindNextFile(hFind, &ffd) != 0);
//
//        FindClose(hFind);
//    }
//
//#else
//
//    if (DIR* dp = opendir(dirname))
//    {
//        int ubuild = 0;
//        dirent* dirp;
//        while ((dirp = readdir(dp)) != NULL)
//            if (sscanf(dirp->d_name, scanname, &ubuild) == 1 && (!iCoreNumber || ubuild <= iCoreNumber))
//            {
//                updates[ubuild] = UpdatesPair(dirp->d_name, section);
//            }
//
//        closedir(dp);
//    }
//
//#endif
//}
//
//void LoadLocaleMPQFiles(int const locale)
//{
//    char filename[2048];
//
//    //// first base old version of dbc files
//    //sprintf(filename, "%s/Data/%s/locale-%s.MPQ", input_path, Locales[locale], Locales[locale]);
//
//    HANDLE localeMpqHandle = nullptr;
//
//    //if (!OpenArchive(filename, &localeMpqHandle))
//    //{
//    //    printf("Error open archive: %s\n\n", filename);
//    //    return;
//    //}
//
//    switch (iCoreNumber) {
//        case CLIENT_TBC:
//        case CLIENT_WOTLK:
//            for (int i = 1; i < 5; ++i)
//            {
//                char ext[3] = "";
//                if (i > 1)
//                {
//                    sprintf(ext, "-%i", i);
//                }
//
//                sprintf(filename, "%s/Data/%s/patch-%s%s.MPQ", input_path, Locales[locale], Locales[locale], ext);
//                if (ClientFileExists(filename) && !OpenArchive(filename))
//                {
//                    //printf("\nError opening patch archive: %s\n", filename);
//                }
//            }
//            break;
//        case CLIENT_CATA:
//        case CLIENT_MOP:
//            // prepare sorted list patches in locale dir and Data root
//            Updates updates;
//            // now update to newer view, locale
//            AppendPatchMPQFilesToList(Locales[locale], Locales[locale], NULL, updates);
//            // now update to newer view, root
//            AppendPatchMPQFilesToList(NULL, NULL, Locales[locale], updates);
//
//            // ./Data wow-update-base files
//            for (int i = 0; Builds[i] && Builds[i] <= CONF_TargetBuild; ++i)
//            {
//                sprintf(filename, "%s/Data/wow-update-base-%u.MPQ", input_path, Builds[i]);
//
//                printf("\n Patching: %s", filename);
//
//                //if (!OpenArchive(filename))
//                if (!SFileOpenPatchArchive(localeMpqHandle, filename, "", 0))
//                {
//                    printf("\nError open patch archive: %s\n", filename);
//                }
//            }
//
//            for (Updates::const_iterator itr = updates.begin(); itr != updates.end(); ++itr)
//            {
//                if (!itr->second.second)
//                {
//                    sprintf(filename, "%s/Data/%s/%s", input_path, Locales[locale], itr->second.first.c_str());
//                }
//                else
//                {
//                    sprintf(filename, "%s/Data/%s", input_path, itr->second.first.c_str());
//                }
//
//                printf("\n Patching: %s", filename);
//
//                //if (!OpenArchive(filename))
//                if (!SFileOpenPatchArchive(localeMpqHandle, filename, itr->second.second ? itr->second.second : "", 0))
//                {
//                    printf("\nError open patch archive: %s\n", filename);
//                }
//            }
//
//            // ./Data/Cache patch-base files
//            for (int i = 0; Builds[i] && Builds[i] <= CONF_TargetBuild; ++i)
//            {
//                sprintf(filename, "%s/Data/Cache/patch-base-%u.MPQ", input_path, Builds[i]);
//
//                printf("\n Patching: %s", filename);
//
//                //if (!OpenArchive(filename))
//                if (!SFileOpenPatchArchive(localeMpqHandle, filename, "", 0))
//                {
//                    printf("\nError open patch archive: %s\n", filename);
//                }
//            }
//
//            // ./Data/Cache/<locale> patch files
//            for (int i = 0; Builds[i] && Builds[i] <= CONF_TargetBuild; ++i)
//            {
//                sprintf(filename, "%s/Data/Cache/%s/patch-%s-%u.MPQ", input_path, Locales[locale], Locales[locale], Builds[i]);
//
//                printf("\n Patching: %s", filename);
//
//                //if (!OpenArchive(filename))
//                if (!SFileOpenPatchArchive(localeMpqHandle, filename, "", 0))
//                {
//                    printf("\nError open patch archive: %s\n", filename);
//                }
//            }
//            break;
//    }
//    printf("\n");
//}
//
///**
// * @brief
// *
// */
//void LoadCommonMPQFiles(int client)
//{
//    std::stringstream filename;
//    std::vector<std::string> kList;
//    switch (client)
//    {
//    case CLIENT_CLASSIC:
//        kList = kClassicMPQList;
//        break;
//    case CLIENT_TBC:
//        kList = kTBCMPQList;
//        break;
//    case CLIENT_WOTLK:
//        kList = kWOTLKMPQList;
//        break;
//    case CLIENT_CATA:
//        kList = kCATAMPQList;
//        break;
//    case CLIENT_MOP:
//        kList = kMOPMPQList;
//        break;
//    }
//
//    char thisName[1024];
//    for (int i = (kList.size() - 1); i >= 0; i--)
//    {
//        sprintf(thisName, "%s/Data/%s", input_path, kList[i].c_str());
//        printf(" Loading MPQ: %s\n", filename.str().c_str());
//        if (ClientFileExists(filename.str().c_str()))
//        {
////            HANDLE fileHandle;
////            if (!OpenArchive(filename, &fileHandle))
////            {
////                printf("Error open archive: %s\n\n", filename);
////            }
//            //new MPQFile(fileHandle, filename);
//        }
//        else
//        {
//            printf("Couldn't find: %s\n\n", filename.str().c_str());
//        }
//    }
//
//    char dirname[1024];
//    struct stat info;
//    string locale;
//    for (int i = 0; i < LOCALES_COUNT; i++)
//    {
//        sprintf(dirname, "%s/Data/%s/locale-%s.MPQ", input_path, Locales[i], Locales[i]);
//        if (ClientFileExists(dirname))
//        {
//            locale = Locales[i];
//            printf("Detected locale: %s\n", locale.c_str());
//            //break;
//        }
//    }
//
//    printf("\n 4: LoadCommonMPQFiles    \n");
//
//    // Loading patches in reverse-order.
//    for (int i = (kList.size() - 1); i >= 0; i--)
//    {
//        filename.str("");
//        filename << input_path << "/Data/" << kList[i];
//        // %s elemets with real language
//        std::string localefile = std::regex_replace(filename.str(), std::regex("\\%s"), locale);
//        // Definitive filename.
//        //sprintf_s(localefile, "%s/Data/%s", input_path, filename);
//        printf("Loading archive %s\n", localefile.c_str());
//        //if (ClientFileExists(localefile.c_str()))
//        //{
//        //    HANDLE fileHandle;
//        //    /// we open multiple file handler without close?
//        //    if (!OpenArchive(localefile.c_str(), &fileHandle))
//        //    {
//        //        printf("Error open archive: %s\n\n", localefile.c_str());
//        //    }
//        //}
//        //else
//        //{
//        //    printf("Couldn't find: %s\n\n", localefile.c_str());
//        //}
//    }
//
//    printf("\n 5: LoadCommonMPQFiles    \n");
//}


/// <summary>
/// The function searches through a list of MPQ archives for files that match a given file mask
/// (e.g., "*.dbc") and appends the found files to a provided list.
/// It handles the extraction of file metadata and ensures that the files are correctly categorized based on their subfolder status.
/// </summary>
/// <param name="fileList">A list of filenames which is populated from the searched files</param>
/// <param name="fileMask">The filemask of the files to be found</param>
void AppendFileListTo(std::vector<dataFile>& fileList, const char* fileMask)
{
    SFILE_FIND_DATA findFileData;
    int counter =0;
    for (int i = 0; i < FinalMPQList.size(); ++i)
    {
        try
        {
            HANDLE searchHandle = SFileFindFirstFile(FinalMPQList[i].fileHandle, fileMask, &findFileData, NULL);
            if (searchHandle)
            {
                dataFile thisFile{};
                thisFile.fileName = findFileData.cFileName;
                thisFile.parentFilename = FinalMPQList[counter].fileName.c_str();
                thisFile.subfolderPath = "";
                thisFile.inSubfolder = false;
                thisFile.mpqId = FinalMPQList[i].lookupId;
                if (FinalMPQList[counter].inSubfolder)
                {
                    thisFile.subfolderPath = FinalMPQList[counter].subfolderPath.c_str();
                    thisFile.inSubfolder = true;
                }

                if (!debugLog)
                {
                    printf("   Adding: %s  from: %s\n", thisFile.fileName.c_str(), FinalMPQList[counter].fileName.c_str());
                }
                else
                {
                    printf("   Adding: %s  subfolder: %s  from: %s  MPQ: %s\n", thisFile.fileName.c_str(), thisFile.subfolderPath.c_str(), FinalMPQList[counter].parentFilename.c_str(), FinalMPQList[counter].fileName.c_str());
                }

                fileList.push_back(thisFile);
                while (SFileFindNextFile(searchHandle, &findFileData))
                {
                    dataFile thisFile{};
                    thisFile.fileName = findFileData.cFileName;
                    thisFile.parentFilename = FinalMPQList[counter].fileName.c_str();
                    thisFile.subfolderPath = "";
                    thisFile.inSubfolder = false;
                    thisFile.mpqId = FinalMPQList[i].lookupId;
                    if (FinalMPQList[counter].inSubfolder)
                    {
                        thisFile.subfolderPath = FinalMPQList[counter].subfolderPath.c_str();
                        thisFile.inSubfolder = true;
                    }
                    if (!debugLog)
                    {
                        printf("   Adding: %s  from: %s\n", thisFile.fileName.c_str(), FinalMPQList[counter].fileName.c_str());
                    }
                    else
                    {
                        printf("   Adding: %s  subfolder: %s  from: %s  MPQ: %s\n", thisFile.fileName.c_str(), thisFile.subfolderPath.c_str(), FinalMPQList[counter].parentFilename.c_str(), FinalMPQList[counter].fileName.c_str());
                    }
                    fileList.push_back(thisFile);
                }
                SFileFindClose(searchHandle);
            }
        }
        catch (std::exception& ex)
        {
        }
        counter += 1;
    }
}

/// <summary>
/// The function extracts files (such as DBC files) from MPQ archives and saves them to a specified local directory.
/// It handles the creation of necessary directories and checks if the files already exist before extracting them.
/// </summary>
/// <param name="dbcFiles">A vector of dataFile structures representing the files to be extracted.
/// Each extractFile contains information about the file, such as its name,
/// parent filename, subfolder path, and whether it is in a subfolder.</param>
/// <param name="mpqPath">A string representing the path within the MPQ archive where the files are located.</param>
/// <param name="fileMask">A string representing the file mask (e.g., "*.dbc") used to filter the files to be extracted.</param>
/// <param name="localPath">A string representing the local directory where the extracted files will be saved.</param>
/// <param name="trimLength">DBC files required the path being trimmed, this flag sets that.</param>
/// <returns></returns>
int ExtractFilefromMPQ(std::vector<dataFile>& dbcFiles, const char * mpqPath,string fileMask,string localPath, bool trimLength)
{
    if (debugLog)
    {
        printf("  Creating directory: %s\n", localPath.c_str());
    }
    CreateDir(localPath);

    // extract DBCs
    for (int i = 0; i < dbcFiles.size(); ++i)
    {
        std::string filename = localPath;

        if (dbcFiles[i].inSubfolder)
        {
            filename += dbcFiles[i].subfolderPath.c_str();
            filename += "/";
            CreateDir(filename);
            if (debugLog)
            {
                printf("  Creating subdirectory: %s  (%s) (%s)  Flag: %s\n", dbcFiles[i].subfolderPath.c_str(), dbcFiles[i].fileName.c_str(), dbcFiles[i].parentFilename.c_str(), dbcFiles[i].inSubfolder ? "true" : "false");
            }
        }
        if (trimLength)
        {
            filename += (dbcFiles[i].fileName.c_str() + strlen(mpqPath));
        }
        else
        {
            filename += (dbcFiles[i].fileName.c_str());
        }
        if (ClientFileExists(filename.c_str()))
        {
            if (debugLog)
            {
                printf("  File found: %s  subfolder: %s \n", filename.c_str(), dbcFiles[i].subfolderPath.c_str());
            }
        }
        else
        {
             //for (int z = 0; z < mpqfiles.size(); ++z)
             //{
                if (ExtractFile(dbcFiles[i].fileName.c_str(), filename, FinalMPQList[dbcFiles[i].mpqId].fileHandle))
                {
                    if (debugLog)
                    {
                        printf("  Extracting %s  subfolder: %s \n", filename.c_str(), dbcFiles[i].subfolderPath.c_str());
                    }
                }
            //}
        }
    }
    //return count;
    return dbcFiles.size();
}

/// <summary>
/// The ExtractWDTFilefromMPQ function is responsible for extracting WDT (World Data Table) files
/// from MPQ archives and saving them to a specified local directory.
/// </summary>
/// <param name="mpqFilePath">The path within the MPQ archive where the WDT files are located.</param>
/// <param name="localPath">The local directory where the extracted WDT files will be saved.</param>
/// <returns>int<returns>The number of WDT files successfully extracted.
int ExtractWDTFilefromMPQ(string mpqFilePath, string localPath)
{
    bool fileFound = false;
    int count =0;
    if (debugLog)
    {
        printf("  Creating directory: %s\n", localPath.c_str());
    }
    CreateDir(localPath);

    // extract DBCs
    for (int i = 0; i < WDTFiles.size(); ++i)
    {
        std::string outputFilename = localPath;

        outputFilename += WDTFiles[i].fileName;

        std::string mpqFilename = mpqFilePath;
        mpqFilename.append(WDTFiles[i].subfolderPath);
        mpqFilename.append(WDTFiles[i].fileName);

        if (ClientFileExists(outputFilename.c_str()))
        {
            fileFound = true;
        }
        else
        {
            if (ExtractFile(mpqFilename.c_str(), outputFilename.c_str(), FinalMPQList[WDTFiles[i].mpqId].fileHandle))
            {
                fileFound = true;
            }
        }
    }

    if (fileFound)
    {
        count = count + 1;
    }
    return count;
}

/**
 * @brief Extracts ADT files from an MPQ archive.
 *
 * @param mpqFilePath Path within the MPQ archive.
 * @param localPath Local directory to save the extracted files.
 * @return int Number of files extracted.
 */
int ExtractADTFilesfromMPQ(string mpqFilePath, string localPath)
{
    bool fileFound = false;
    int count =0;
    if (debugLog)
    {
        printf("   Creating directory: %s\n", localPath.c_str());
    }
    CreateDir(localPath);

    char mpq_filename[2048];
    char output_filename[2048];
    char mpq_map_name[2048]{};

    // extract Maps
    //for (int i = 0; i < 1; ++i)
    for (uint32 i = 0; i < ADTFiles.size(); ++i)
    {
        printf("   (%03i/%03i) Extracting map Id: %04i Name: %s (%s) ADT files\n",i+1, (int)ADTFiles.size(), (int)ADTFiles[i].lookupId, ADTFiles[i].fileName.c_str(), ADTFiles[i].displayName.c_str());
        // Loop Through x coords
        for (uint32 xcoord = 0; xcoord < WDT_MAP_SIZE; xcoord++)
        {
            printf("   Processing...........%d%%\r", (100 * (xcoord + 1)) / WDT_MAP_SIZE);

            // Loop Through y coords
            for (uint32 ycoord = 0; ycoord < WDT_MAP_SIZE; ycoord++)
            {
                //printf ("y: %02i\n", ycoord);
                try
                {
                    // base _obj0.adt
                    std::string outputFilename = localPath;
                    outputFilename += ADTFiles[i].fileName;
                    outputFilename += "_";
                    outputFilename += to_string(xcoord);
                    outputFilename += "_";
                    outputFilename += to_string(ycoord);
                    outputFilename += "_obj0.adt";

                    std::string mpqFilename = mpqFilePath;
                    mpqFilename.append(ADTFiles[i].subfolderPath);
                    mpqFilename.append(ADTFiles[i].fileName);
                    mpqFilename += "_";
                    mpqFilename += to_string(xcoord);
                    mpqFilename += "_";
                    mpqFilename += to_string(ycoord);
                    mpqFilename += "_obj0.adt";

                    if (ClientFileExists(outputFilename.c_str()))
                    {
                        count += 1;
                    }
                    else
                    {
                        try
                        {
                            if (ExtractFile(mpqFilename.c_str(), outputFilename.c_str(), FinalMPQList[ADTFiles[i].mpqId].fileHandle))
                            {
                                  count += 1;
                            }
                        }
                        catch (const std::exception&)
                        {
                            printf("   ERROR: ExtractFile failed with an exception");
                        }

                    }
                    sprintf(mpq_filename, "adt/%s_%u_%u_obj0.adt", ADTFiles[i].fileName.c_str(), xcoord, ycoord);
                    sprintf(output_filename, "%s/maps/%04u%02u%02u.map", output_path.c_str(), ADTFiles[i].lookupId, ycoord, xcoord);

                    try
                    {
                        ConvertADT(mpq_filename, output_filename);// , y, x);
                        ParseMapFiles(i, xcoord, ycoord, mpq_filename);
                    }
                    catch (const std::exception&)
                    {
                        printf("   ERROR: ConvertADT failed with an exception");
                    }

                    // base _obj1.adt
                    outputFilename = localPath;
                    outputFilename += ADTFiles[i].fileName;
                    outputFilename += "_";
                    outputFilename += to_string(xcoord);
                    outputFilename += "_";
                    outputFilename += to_string(ycoord);
                    outputFilename += "_obj1.adt";

                    mpqFilename = mpqFilePath;
                    mpqFilename.append(ADTFiles[i].subfolderPath);
                    mpqFilename.append(ADTFiles[i].fileName);
                    mpqFilename += "_";
                    mpqFilename += to_string(xcoord);
                    mpqFilename += "_";
                    mpqFilename += to_string(ycoord);
                    mpqFilename += "_obj1.adt";

                    if (ClientFileExists(outputFilename.c_str()))
                    {
                              count += 1;
                    }
                    else
                    {
                        if (ExtractFile(mpqFilename.c_str(), outputFilename.c_str(), FinalMPQList[ADTFiles[i].mpqId].fileHandle))
                        {
                              count += 1;
                        }
                    }

                    // base .adt
                    outputFilename = localPath;
                    outputFilename += ADTFiles[i].fileName;
                    outputFilename += "_";
                    outputFilename += to_string(xcoord);
                    outputFilename += "_";
                    outputFilename += to_string(ycoord);
                    outputFilename += ".adt";

                    mpqFilename = mpqFilePath;
                    mpqFilename.append(ADTFiles[i].subfolderPath);
                    mpqFilename.append(ADTFiles[i].fileName);
                    mpqFilename += "_";
                    mpqFilename += to_string(xcoord);
                    mpqFilename += "_";
                    mpqFilename += to_string(ycoord);
                    mpqFilename += ".adt";

                    if (ClientFileExists(outputFilename.c_str()))
                    {
                              count += 1;
                    }
                    else
                    {
                        try
                        {
                            if (ExtractFile(mpqFilename.c_str(), outputFilename.c_str(), FinalMPQList[ADTFiles[i].mpqId].fileHandle))
                            {
                                  count += 1;
                            }
                        }
                        catch (const std::exception&)
                        {
                            printf("   ERROR: ExtractFile failed with an exception");
                        }

                    }

                    sprintf(mpq_filename, "adt/%s_%u_%u.adt", ADTFiles[i].fileName.c_str(), xcoord, ycoord);
                    sprintf(output_filename, "%s/maps/%04u%02u%02u.map", output_path.c_str(), ADTFiles[i].lookupId, ycoord, xcoord);

                    try
                    {
                        ConvertADT(mpq_filename, output_filename);// , y, x);
                        ParseMapFiles(i, xcoord, ycoord, mpq_filename);
                    }
                    catch (const std::exception&)
                    {
                        printf("   ERROR: ConvertADT failed with an exception");
                    }
                }
                catch (const std::exception&)
                {
                    printf("   ERROR: ExtractADTFilesfromMPQ failed with an exception");
                }

            }
        }
    }
    return count;
}

//// Really need to get this working, so we don't have to go hunting through MPQ's again when the file is already extracted
//uint32 ReadMapDBC(std::string dbcPath)
//{
//    bool openOk = false;
//    std::string thisPath = dbcPath;
//    thisPath += "Map.dbc";
//
//    FILE* dbcFile;
//#ifdef WIN32
//    if (fopen_s(&dbcFile, thisPath.c_str(), "rb") == 0)
//    {
//        openOk = true;
//    }
//#else
//    if ((pFile = fopen(tmp1, "rb")))
//    {
//        openOk = true
//    }
//#endif
//    if (openOk)
//    {
//        DBCFile dbc(dbcFile);
//        if (!dbc.open())
//        {
//            printf("Error opening DBC file: %s\n", thisPath.c_str());
//            return 0;
//        }
//        else
//        {
//            size_t map_count = dbc.getRecordCount();
//            map_ids = new map_id[map_count];
//            for (uint32 x = 0; x < map_count; ++x)
//            {
//                map_ids[x].id = dbc.getRecord(x).getUInt(0);
//                strcpy(map_ids[x].name, dbc.getRecord(x).getString(1));
//            }
//
//            printf(" Success!! - %u maps loaded\n", uint32(map_count));
//            //SFileCloseFile(dbcFile);
//            return map_count;
//        }
//    }
//    else
//    {
//        printf("Error opening file map.dbc: %s\n", thisPath.c_str());
//        return 0;
//    }
//}

/**
 * @brief Reads DBC files from an MPQ archive.
 *
 * @param fileName Name of the DBC file to read.
 * @param mapList Vector to store the read data.
 * @param dbcType Type of DBC file.
 */
void NewReadDbcFromMPQ(const char* fileName, std::vector<dataFile>& mapList, int dbcType)
{
    SFILE_FIND_DATA findFileData;
    for (int i = 0; i < FinalMPQList.size(); ++i)
    {
        try
        {
            HANDLE searchHandle = SFileFindFirstFile(FinalMPQList[i].fileHandle, fileName, &findFileData, NULL);
            if (searchHandle)
            {
                HANDLE dbcFile;
                if (OpenNewestFile(fileName, &dbcFile))
                {
                    printf("\n   Found %s in archive!, Reading data... \n",fileName);
                }

                DBCFile dbc(dbcFile);
                if (dbc.open())
                {
                    size_t dbcFile_count = dbc.getRecordCount();
                    size_t maxid = dbc.getMaxId();
                    uint16* buffer = new uint16[maxid + 1];


                    for (uint32 x = 0; x < dbcFile_count; ++x)
                    {
                        dataFile dbc_record;
                        std::string tempString = "";

                        // These are used for area table processing where the fields move about
                        int idField = 0;
                        int valueField = 3;
                        int nameField = 0;
                        int liquidField = 3;

                        switch (dbcType)
                        {
                            case 1: // Maps.dbc
                                dbc_record.lookupId = dbc.getRecord(x).getUInt(0);      // Map Id
                                dbc_record.mpqId = FinalMPQList[i].lookupId;                // MPQ Id that the dbc is in
                                dbc_record.fileName = dbc.getRecord(x).getString(1);    // Map Folder Name
                                switch (iCoreNumber)
                                {
                                    case CLIENT_CLASSIC:
                                        nameField = 4;
                                        break;
                                    case CLIENT_TBC:
                                        nameField = 4;
                                        break;
                                    case CLIENT_WOTLK:
                                        nameField = 5;
                                        break;
                                    case CLIENT_CATA:
                                        nameField = 6;
                                        break;
                                        case CLIENT_MOP:
                                        nameField = 5;
                                        break;
                                    default:
                                        break;
                                }
                                tempString = dbc.getRecord(x).getString(nameField);
                                if (tempString.length() == 0)
                                {
                                    tempString = dbc_record.fileName;
                                }
                                else
                                {
                                    dbc_record.displayName = tempString;                    // Map Display Name
                                }
                                dbc_record.uint16Value = dbc.getRecord(x).getUInt(2);   // Map Type
                                break;
                            case 2: // AreaTable.dbc
                                if (maxAreaId < maxid)
                                {
                                    maxAreaId = maxid;
                                }

                                // TODO: Localised AreaTable.dbc may have the areaname in another column
                                switch (iCoreNumber)
                                {
                                    case CLIENT_CLASSIC:
                                        nameField = 11;
                                        break;
                                    case CLIENT_TBC:
                                        nameField = 11;
                                        break;
                                    case CLIENT_WOTLK:
                                        nameField = 11;
                                        break;
                                    case CLIENT_CATA:
                                        nameField = 11;
                                        break;
                                        case CLIENT_MOP:
                                        nameField = 13;
                                        break;
                                    default:
                                        break;
                                }
                                dbc_record.lookupId = dbc.getRecord(x).getUInt(idField);       // Area Id
                                dbc_record.uint16Value = dbc.getRecord(x).getUInt(valueField); // Area Bit
                                dbc_record.mpqId = FinalMPQList[i].lookupId;                       // MPQ Id that the dbc is in
                                dbc_record.fileName = dbc.getRecord(x).getString(nameField);   // Area Name

                                break;
                            case 3: // LiquidType.dbc
                                try
                                {
                                    dbc_record.lookupId = dbc.getRecord(x).getUInt(0);                       // Liquid Id
                                    switch (iCoreNumber)
                                    {
                                        case CLIENT_CLASSIC:
                                            liquidField = 2;
                                            break;
                                        case CLIENT_TBC:
                                            liquidField = 2;
                                            break;
                                        case CLIENT_WOTLK:
                                            liquidField = 3;
                                            break;
                                        case CLIENT_CATA:
                                            liquidField = 3;
                                            break;
                                            case CLIENT_MOP:
                                            liquidField = 3;
                                            break;
                                        default:
                                            break;
                                    }

                                    dbc_record.uint16Value = dbc.getRecord(x).getUInt(liquidField);          // Base Liquid Type Id
                                    dbc_record.mpqId = FinalMPQList[i].lookupId;                             // MPQ Id that the dbc is in
                                    dbc_record.fileName = dbc.getRecord(x).getString(1);                     // Liquid Name

                                    if (dbc_record.uint16Value>3)
                                    {
                                        printf("   ERROR: NewReadDbcFromMPQ (LiquidTypeId invalid): %i\n", dbc_record.uint16Value );
                                        dbc_record.uint16Value = 0;
                                    }
                                    dbc_record.displayName = LiquidTypeList[dbc_record.uint16Value].c_str(); // Base Liquid Name
                                }
                                catch (std::exception& ex)
                                {
                                    printf("   ERROR: NewReadDbcFromMPQ (displayname) failed with an exception: %s\n", ex.what());
                                }
                                break;
                            default:
                                printf("   ERROR: NewReadDbcFromMPQ (unknown dbcType): %i\n", dbcType);
                                break;
                        }

                        printf("    Loaded  Id: %04i  Type: %04i  Name: %s (%s)\n", dbc_record.lookupId, dbc_record.uint16Value, dbc_record.fileName.c_str(), dbc_record.displayName.c_str());
                        mapList.push_back(dbc_record);
                    }
                    break;
                }
            }
            SFileFindClose(searchHandle);
        }
        catch (std::exception& ex)
        {
            printf("   ERROR: NewReadDbcFromMPQ failed with an exception: %s\n", ex.what());
        }
    }
}

// The start of VMAP Extras



/**
 * @brief Parses the map files and processes each map.
 */
bool ParseMapFiles(uint32 mapIndex, uint32 xcoord, uint32 ycoord, std::string adt_filename)
{
    char* fn = new char[512];
    char* id = new char[10];
    StringSet failedPaths;

    ADTFile* adt = new ADTFile(adt_filename);
    adt->initLoadFileFromDisk(mapIndex, xcoord, ycoord, failedPaths, iCoreNumber, szRawVMAPMagic, true, szWorkDirWmo, adt_filename);
    //adt->init(mapIndex, xcoord, ycoord, failedPaths, iCoreNumber, szRawVMAPMagic, true, szWorkDirWmo);
    delete adt;

    if (!failedPaths.empty())
    {
        printf(" Warning: Some models could not be extracted, see below\n");
        for (StringSet::const_iterator itr = failedPaths.begin(); itr != failedPaths.end(); ++itr)
        {
            printf("Could not find file of model %s\n", itr->c_str());
        }
        printf(" A few not found models can be expected and are not alarming.\n");
    }

    delete[] fn;
    delete[] id;
}


/**
 * @brief Parses the map files and processes each map.
 */
static void OldParseMapFiles()
{
    char* fn = new char[512];
    char* id = new char[10];
    StringSet failedPaths;
    printf("\n");
    for (unsigned int i = 0; i < MapList.size(); ++i)
    {
        sprintf(id, "%04u", MapList[i].lookupId);
        sprintf(fn, "World\\Maps\\%s\\%s.wdt", MapList[i].fileName.c_str(), MapList[i].fileName.c_str());

        HANDLE handleWDT = nullptr;
        //if (!OpenNewestFile(fn, &handleWDT))
        //{
        //    printf("Error opening WDT file %s\n", fn);
        //    continue;
        //}
        //handleWDT = WDTFiles[i].fileHandle;

        ////thisWDTFile.loadFileFromDisk(WDTFiles[i].fileName);
        ////WDTFile WDT(handleWDT, fn, MapList[i].fileName.c_str());
        //for (int i = 0; i < WDTFiles.size(); ++i)
        //{

        //    if (WDT.init(id, MapList[i].lookupId, szWorkDirWmo))
        //    {
        //        printf(" Processing Map %u (%s)\n[", MapList[i].lookupId, MapList[i].fileName);
        //        for (int x = 0; x < 64; ++x)
        //        {
        //            for (int y = 0; y < 64; ++y)
        //            {
        //                if (ADTFile* ADT = WDT.GetMap(x, y))
        //                {
        //                    ADT->init(MapList[i].lookupId, x, y, failedPaths, iCoreNumber, localSzRawVMAPMagic, preciseVectorData, szWorkDirWmo);
        //                    delete ADT;
        //                }
        //            }
        //            printf("#");
        //            fflush(stdout);
        //        }
        //        printf("]\n");
        //    }
        //}
    }

    if (!failedPaths.empty())
    {
        printf(" Warning: Some models could not be extracted, see below\n");
        for (StringSet::const_iterator itr = failedPaths.begin(); itr != failedPaths.end(); ++itr)
        {
            printf("Could not find file of model %s\n", itr->c_str());
        }
        printf(" A few not found models can be expected and are not alarming.\n");
    }

    delete[] fn;
    delete[] id;
}

///**
// * @brief Displays the usage information for the program.
// *
// * @param prg The program name.
// */
//void UsageVmap(char* prg)
//{
//    printf(" Usage: %s [OPTION]\n\n", prg);
//    printf(" Extract client database files and generate map files.\n");
//    printf("   -h, --help            show the usage\n");
//    printf("   -i, --input <path>     search path for game client archives\n");
//    printf("   -s, --small           extract smaller vmaps by optimizing data. Reduces\n");
//    printf("                         size by ~ 500MB\n");
//    printf("\n");
//    printf(" Example:\n");
//    printf(" - use data path and create larger vmaps:\n");
//    printf("   %s -l -i \"c:\\games\\world of warcraft\"\n", prg);
//}
//
///**
// * @brief Processes the command line arguments.
// *
// * @param argc The number of arguments.
// * @param argv The array of arguments.
// * @return true if the arguments were processed successfully, false otherwise.
// */
//bool processArgvVmap(int argc, char** argv)
//{
//    bool result = true;
//    char* param = NULL;
//
//    for (int i = 1; i < argc; ++i)
//    {
//        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 )
//        {
//            result = false;
//            break;
//        }
//        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--small") == 0 )
//        {
//            result = true;
//        }
//        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0 )
//        {
//            param = argv[++i];
//            if (!param)
//            {
//                result = false;
//                break;
//            }
//
//            result = true;
//            strcpy(input_path, param);
//        }
//        else
//        {
//            result = false;
//            break;
//        }
//    }
//
//    if (!result)
//    {
//        Usage(argv[0]);
//    }
//    return result;
//}
//
//
////xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//// Main
////
//// The program must be run with two command line arguments
////
//// Arg1 - The source MPQ name (for testing reading and file find)
//// Arg2 - Listfile name
////
//
//int mainVmap(int argc, char** argv)
//{
//    // Use command line arguments, when some
//    if (!processArgvVmap(argc, argv))
//    {
//        return 1;
//    }
//
//    int thisBuild = getBuildNumber(input_path);
//    iCoreNumber = getCoreNumberFromBuild(thisBuild);
//    std::string outDir = std::string(output_path) + "/vmaps";
//
//    showBanner("Vertical Map Asset Extractor", iCoreNumber);
//    setVMapMagicVersion(iCoreNumber, szRawVMAPMagic);
//    showWebsiteBanner();
//
//    bool success = true;
//    std::string sdir = std::string(szWorkDirWmo) + "/dir";
//    std::string sdir_bin = std::string(szWorkDirWmo) + "/dir_bin";
//    struct stat status;
//    bool dirty = false;
//
//    if (!stat(sdir.c_str(), &status) || !stat(sdir_bin.c_str(), &status))
//    {
//        printf(" Your %s directory seems to exist, please delete it!\n", szWorkDirWmo.c_str());
//        dirty = true;
//    }
//
//    if (!stat(outDir.c_str(), &status))
//    {
//        printf(" Your %s directory seems to exist, please delete it!\n", outDir.c_str());
//        dirty = true;
//    }
//
//    if (dirty)
//    {
//        printf(" <press return to exit>");
//        char garbage[2];
//        int ret = scanf("%c", garbage);
//        return 1;
//    }
//
//    printf(" Beginning work ....\n");
//    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//    // Create the working and ouput directories
//    CreateDir(std::string(szWorkDirWmo));
//    CreateDir(outDir);
//
//    // prepare archive name list
//    LoadCommonMPQFiles(iCoreNumber);
//
//    if (gOpenArchives.empty())
//    {
//        printf("FATAL ERROR: None MPQ archive found by path '%s'. Use -d option with proper path.\n", input_path);
//        return 1;
//    }
//    //if (iCoreNumber == CLIENT_CLASSIC)
//    //{
//    //    ReadLiquidTypeTableDBC();
//    //}
//
//    // extract data
//    if (success)
//    {
//        success = ExtractWmo(iCoreNumber, szRawVMAPMagic, preciseVectorData, szWorkDirWmo);
//    }
//
//    // Open map.dbc
//    if (success)
//    {
//        //HANDLE dbcFile;
//        //if (!OpenNewestFile("DBFilesClient\\Map.dbc", &dbcFile))
//        //{
//        //    printf("Error: Cannot find Map.dbc in archive!\n");
//        //    exit(1);
//        //}
//
//        //printf("Found Map.dbc in archive!\n");
//        printf("\n Reading maps from Map.dbc... ");
//
//        //DBCFile dbc(dbcFile);
//        //if (!dbc.open())
//        //{
//        //    printf("Fatal error: Could not read Map.dbc!\n");
//        //    exit(1);
//        //}
//
//        //map_count = dbc.getRecordCount();
//        //map_ids = new map_id[map_count];
//
//        for (unsigned int x = 0; x < MapList.size(); ++x)
//        {
////            MapList[x].lookupId = dbc.getRecord(x).getUInt(0);
//  //          strcpy(map_ids[x].name, dbc.getRecord(x).getString(1));
//            printf(" Map %d - %s\n", MapList[x].lookupId, MapList[x].fileName);
//        }
//
//
//        ParseMapFiles(szRawVMAPMagic);
//        //delete [] map_ids;
//        //nError = ERROR_SUCCESS;
//        // Extract models, listed in DameObjectDisplayInfo.dbc
//        ExtractGameobjectModels(iCoreNumber, szRawVMAPMagic, preciseVectorData,szWorkDirWmo);
//    }
//
////    delete [] LiqType;
//
//    if (!success)
//    {
//        printf("ERROR: Extract for %s. Work NOT complete.\n   Precise vector data=%d.\nPress any key.\n", szRawVMAPMagic.c_str(), preciseVectorData);
//        getchar();
//        return 1;
//    }
//
//    success = AssembleVMAP(std::string(szWorkDirWmo), outDir, szRawVMAPMagic);
//
//    if (!success)
//    {
//        printf("ERROR: VMAP building for %s NOT completed", szRawVMAPMagic.c_str());
//        getchar();
//        return 1;
//    }
//
//    printf("\n");
//    printf(" VMAP building complete. No errors.\n");
//
//    return 0;
//}
//
//
//












/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char** argv)
{
    HandleArgs(argc, argv);

    // These need to be set before the banner is shown to reflect the correct information
    iBuildNumber = getBuildNumber(input_path.c_str());                  // Get the build number of the client from wow.exe
    iCoreNumber = getCoreNumberFromBuild(iBuildNumber);         // Get the core number of the client from the build number

    showBanner("DBC Extractor & Map Generator", iCoreNumber);
    showWebsiteBanner();

    printf("\n");
    printf("  Selected Options: \n");
    printf("  Input Path: %s\n", input_path.c_str());
    printf("  Output Path: %s\n", output_path.c_str());
    printf("  Extract dbc: %s\n", (CONF_extract | EXTRACT_DBC_EXTRACTORS_ALL) ? "true" : "false");
    printf("  Extract maps: %s\n", (CONF_extract | EXTRACT_MAP) ? "true" : "false");

    // Stage 1: Get the build number and core number of the client

    printf("\n");
    printf(" Stage 1: Get the build number and core number of the client\n");
    printf(" ===========================================================\n");
    printf("  Build Number: %d\n", iBuildNumber);     // iBuildNumber contains the build number of the client
    printf("  Core Number: %d\n", iCoreNumber);       // iCoreNumber contains the core number of the client

    MAP_VERSION_MAGIC = setMapMagicVersion(iCoreNumber);
    szRawVMAPMagic = setVMapMagicVersion(iCoreNumber);

    std::cout << "Press ENTER to continue...";
    std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );

    // Stage 2: Create a list of MPQ's needed for this core
    std::vector<std::string> MPQList = getMPQListForCore(iCoreNumber);

    if (MPQList.empty())
    {
        exit(1);
    }
    else
    {
        for (int i = (MPQList.size() - 1); i >= 0; i--)
        {
            std::string LocaleString = MPQList[i].c_str();
            if (!LocaleString.find("%"))
            {
                if (iCoreNumber == CLIENT_CLASSIC)
                {
                    LANG_COUNT=1;
                }
                for (int iThisLocale = 0; iThisLocale < LANG_COUNT; iThisLocale++)
                {
                    std::string tmp2 = input_path;
                    tmp2.append ("/Data/");
                    if (iCoreNumber != CLIENT_CLASSIC)
                    {
                        tmp2.append(std::regex_replace(LocaleString, std::regex("%s"), Locales[iThisLocale]));
                    }
                    if (ClientFileExists(tmp2.c_str()))
                    {
                        dataFile thisFile;
                        thisFile.fileName = tmp2.c_str();
                        thisFile.subfolderPath = Locales[iThisLocale];
                        thisFile.inSubfolder = true;

                        FinalMPQList.push_back(thisFile);
                    }
                }
            }
            else
            {
                std::string tmp2 = input_path;
                tmp2.append("/Data/");
                tmp2.append(MPQList[i].c_str());
                if (ClientFileExists(tmp2.c_str()))
                {
                    dataFile thisFile;
                    thisFile.fileName = tmp2.c_str();
                    thisFile.subfolderPath = "";
                    thisFile.inSubfolder = false;
                    FinalMPQList.push_back(thisFile);
                }
            }
        }
        printf("\n");

        // Display Final MPQ List
        //printf("  Final MPQ List: \n");

        //for (int i = 0; i < FinalMPQList.size(); ++i)
        //{
        //    printf(" %d: MPQ: %s  Localised folder: %s \n", i, FinalMPQList[i].filename.c_str(), FinalMPQList[i].subfolderPath.c_str());
        //}
    }

    // 2.1: Open the MPQ's
    printf("\n");
    printf(" Stage 2: Open the MPQ's needed for this core\n");
    printf(" ============================================\n");

    int DBCDB2Count = 0;
    int WDTCount = 0;
    int ADTCount = 0;
    for (int i = 0; i < FinalMPQList.size(); ++i)
    {
        HANDLE fileHandle = new HANDLE;
        printf("  %02d: %s - ", i + 1, FinalMPQList[i].fileName.c_str());
        /// we open multiple file handler without close?
        if (!OpenArchive(FinalMPQList[i].fileName.c_str(), &fileHandle))
        {
            if (!debugLog)
            {
                printf("Error !!\n");
            }
            else
            {
                printf("Error opening archive: %s\n\n", FinalMPQList[i].fileName.c_str());
            }
        }
        else
        {
            if (!debugLog)
            {
                printf("Success !!\n");
            }
            else
            {
                printf("Error opening archive: %s\n\n", FinalMPQList[i].fileName.c_str());
            }
            FinalMPQList[i].fileHandle =fileHandle;
        }
    }
    printf("\n");

    printf("  Summary: MPQ Files: %d\n", (int)FinalMPQList.size());

    // Stage 3: Extract the DBC files from the MPQ's
    printf("\n");
    printf(" Stage 3: Extract the DBC / DB2 files from the MPq's\n");
    printf(" ===================================================\n");

    AppendFileListTo(DBCFiles, "*.dbc");
    const char* mpqPath = "DBFilesClient\\";
    DBCDB2Count = ExtractFilefromMPQ(DBCFiles,mpqPath, "*.dbc","dbc/", true);

    AppendFileListTo(DB2Files, "*.db2");
    mpqPath = "DBFilesClient\\";
    DBCDB2Count += ExtractFilefromMPQ(DB2Files, mpqPath, "*.db2","dbc/", true);

    printf("\n");
    printf("  Summary: Detected %i DBC and %i DB2 Files: \n", (int)DBCFiles.size(), (int)DB2Files.size());

    printf("\n");
    printf(" Stage 4: Open dbc's needed by the extractor\n");
    printf(" ===========================================\n");

    NewReadDbcFromMPQ("DBFilesClient\\Map.dbc",MapList, 1);
    //ReadMapDBC("dbc/");

    // Prepare the LiquidTypeList lookup
    LiquidTypeList.push_back("Water");
    LiquidTypeList.push_back("Ocean");
    LiquidTypeList.push_back("Magma");
    LiquidTypeList.push_back("Slime");

    printf("\n");
    printf(" Stage 5: Load Liquid Types from LiquidType.dbc\n");
    printf(" ==============================================\n");

    NewReadDbcFromMPQ("DBFilesClient\\LiquidType.dbc",LiquidList, 3);

    printf("\n");
    printf(" Stage 6: Load Area Types from AreaTable.dbc\n");
    printf(" ===========================================\n");
    NewReadDbcFromMPQ("DBFilesClient\\AreaTable.dbc",AreaList, 2);

    printf("\n");
    printf("  Summary: Maps: %i, Areas: %i, Liquids: %i Loaded: \n", (int)MapList.size(), (int)AreaList.size(), (int)LiquidList.size());

    printf("\n");
    printf(" Stage 7: Extract WDT files needed by the extractor\n");
    printf(" ==================================================\n");

    for (int i = 0; i < MapList.size(); ++i)
    {
        dataFile thisWdtFile;

        std::string thisMapWdtFolder = MapList[i].fileName;
        thisMapWdtFolder.append ("\\");

        std::string thisMapWdtFilename = MapList[i].fileName;
        thisMapWdtFilename.append (".wdt");

        thisWdtFile.fileName = thisMapWdtFilename;
        thisWdtFile.subfolderPath = thisMapWdtFolder;
        thisWdtFile.inSubfolder = true;
        thisWdtFile.lookupId = MapList[i].lookupId;
        thisWdtFile.displayName = MapList[i].displayName;
        WDTFiles.push_back(thisWdtFile);

        dataFile thisAdtFile;
        std::string thisMapAdtFilename = MapList[i].fileName;

        thisAdtFile.fileName = thisMapAdtFilename;
        thisAdtFile.subfolderPath = thisMapWdtFolder;
        thisAdtFile.inSubfolder = true;
        thisAdtFile.lookupId = MapList[i].lookupId;
        thisAdtFile.displayName = MapList[i].displayName;
        ADTFiles.push_back(thisAdtFile);
    }

    for (int i = 0; i < WDTFiles.size(); ++i)
    {
        printf("   (%03i/%03i)  Extracting: %04i  Name: %s\n", i+1, (int)WDTFiles.size(), (int)WDTFiles[i].lookupId, WDTFiles[i].fileName.c_str());
        WDTCount += ExtractWDTFilefromMPQ("world\\maps\\", "wdt/");
    }

    printf("\n");
    printf("  Summary: Detected %i WDT Files, %i processed\n", (int)WDTFiles.size(), WDTCount);

    ////// Stage 5: Extract the adt files from the wdt files
    printf("\n");
    printf(" Stage 8: Extract ADT files needed by the extractor and create .map files\n");
    printf(" ========================================================================\n");
    ADTCount += ExtractADTFilesfromMPQ("world\\maps\\", "adt/");

    printf("\n\n");
    printf("   Summary: Processed %i ADT Files from %i maps\n", ADTCount, (int)ADTFiles.size());

    // Stage 7: Extract the ???? files into buildings



    bool success = true;
    std::string sdir = std::string(szWorkDirWmo) + "/dir";
    std::string sdir_bin = std::string(szWorkDirWmo) + "/dir_bin";
    struct stat status;
    bool dirty = false;

    if (!stat(sdir.c_str(), &status) || !stat(sdir_bin.c_str(), &status))
    {
        printf(" Your %s directory seems to exist, please delete it!\n", szWorkDirWmo.c_str());
        dirty = true;
    }

    if (!stat(outDir.c_str(), &status))
    {
        printf(" Your %s directory seems to exist, please delete it!\n", outDir.c_str());
        dirty = true;
    }

    if (dirty)
    {
        printf(" <press return to exit>");
        char garbage[2];
        int ret = scanf("%c", garbage);
        return 1;
    }

    printf(" Beginning work ....\n");


    // Stage 9: Create VMTile files


    // Stage 10: Create VMTileSet files


    //Stage 11: Create VMMap files


    //Stage 12: Create VMMapSet files


    //Stage 13: Create VMap files


    // Clean up, close down and release all the mpq handles we had stored
    CloseArchives();
    for (int i = 0; i < FinalMPQList.size(); ++i)
    {
        SFILECLOSEARCHIVE(FinalMPQList[i].fileHandle);
    }

    printf("\n\nExtraction Completed !!!\n\n");

    return 0;
}
