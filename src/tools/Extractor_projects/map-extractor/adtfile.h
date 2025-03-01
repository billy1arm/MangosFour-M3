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

#ifndef VMAP_ADTFILE_H
#define VMAP_ADTFILE_H

#include "model.h"

#define TILESIZE (533.33333f)
#define CHUNKSIZE ((TILESIZE) / 16.0f)
#define UNITSIZE (CHUNKSIZE / 8.0f)
typedef std::set<std::string> StringSet;

/**
 * @brief Class representing an ADT (Azeroth Data Terrain) file.
 */
class ADTFile
{
    public:
        /**
         * @brief Constructs an ADTFile object with the given filename.
         *
         * @param filename The name of the ADT file.
         */
        ADTFile(std::string filename);
        /**
         * @brief Destroys the ADTFile object.
         */
        ~ADTFile();
        int nWMO = 0; /**< Number of WMOs (World Map Objects) in the ADT file. */
        int nMDX = 0; /**< Number of MDXs (Model files) in the ADT file. */
        std::string* WmoInstansName = nullptr; /**< Names of the WMO instances. */
        std::string* ModelInstansName = nullptr; /**< Names of the model instances. */
        /**
         * @brief Initializes the ADT file.
         *
         * @param map_num The map number.
         * @param tileX The X coordinate of the tile.
         * @param tileY The Y coordinate of the tile.
         * @param failedPaths The set of failed paths.
         * @param iCoreNumber The core number.
         * @param szRawVMAPMagic The VMAP magic string.
         * @param preciseVectorData Whether to use precise vector data.
         * @param szWorkDirWmo The working directory for WMO files.
         * @return True if the initialization was successful, false otherwise.
         */
        bool init(uint32 map_num, uint32 tileX, uint32 tileY, StringSet& failedPaths, int iCoreNumber, std::string szRawVMAPMagic, bool preciseVectorData, std::string szWorkDirWmo);
        bool initLoadFileFromDisk(uint32 map_num, uint32 tileX, uint32 tileY, StringSet& failedPaths, int iCoreNumber, std::string szRawVMAPMagic, bool preciseVectorData, std::string szWorkDirWmo, std::string localAdtFilename);

    private:
        std::string AdtFilename; /**< The name of the ADT file. */
};

#endif // VMAP_ADTFILE_H
