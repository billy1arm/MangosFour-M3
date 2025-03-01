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

#include <algorithm>
#include <cstdio>
#include "adtfile.h"
#include <ExtractorCommon.h>

/**
 * @brief Constructs an ADTFile object with the given filename.
 *
 * @param filename The name of the ADT file.
 */
ADTFile::ADTFile(std::string filename) : AdtFilename(filename), ModelInstansName(nullptr)
{
}

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
bool ADTFile::init(uint32 map_num, uint32 tileX, uint32 tileY, StringSet& failedPaths, int iCoreNumber, std::string szRawVMAPMagic, bool preciseVectorData, std::string szWorkDirWmo)
{
    HANDLE adtHandle;

    // Open the newest ADT file.
    if (!OpenNewestFile(AdtFilename.c_str(), &adtHandle))
    {
        printf("Error initializing ADT %s\n", AdtFilename.c_str());
    }

    // Create an MPQFile object for the ADT file.
    MPQFile ADT(adtHandle, AdtFilename.c_str());

    // Check if the ADT file is empty.
    if (ADT.isEof())
    {
        return false;
    }

    uint32 size;

    std::string xMap;
    std::string yMap;
    std::string filename = AdtFilename;

    // Remove the ".adt" extension from the filename.
    filename.erase(filename.find(".adt"), 4);
    std::string TempMapNumber;

    // Extract the map coordinates from the filename.
    TempMapNumber = filename.substr(filename.length() - 6, 6);
    xMap = TempMapNumber.substr(TempMapNumber.find("_") + 1, (TempMapNumber.find_last_of("_") - 1) - (TempMapNumber.find("_")));
    yMap = TempMapNumber.substr(TempMapNumber.find_last_of("_") + 1, (TempMapNumber.length()) - (TempMapNumber.find_last_of("_")));
    filename.erase((filename.length() - xMap.length() - yMap.length() - 2), (xMap.length() + yMap.length() + 2));

    std::string AdtMapNumber = xMap + ' ' + yMap + ' ' + GetUniformName(filename);

    // Open the directory file for writing.
    std::string dirname = std::string(szWorkDirWmo) + "/dir_bin";
    FILE* dirfile;
    dirfile = fopen(dirname.c_str(), "ab");
    if (!dirfile)
    {
        printf("Can't open dirfile!'%s'\n", dirname.c_str());
        return false;
    }

    // Read the ADT file and process its chunks.
    while (!ADT.isEof())
    {
        char fourcc[5];
        ADT.read(&fourcc, 4);
        ADT.read(&size, 4);
        flipcc(fourcc);
        fourcc[4] = 0;

        size_t nextpos = ADT.getPos() + size;

        if (!strcmp(fourcc, "MCIN"))
        {
            // Process MCIN chunk.
        }
        else if (!strcmp(fourcc, "MTEX"))
        {
            // Process MTEX chunk.
        }
        else if (!strcmp(fourcc, "MMDX"))
        {
            // Process MMDX chunk.
            if (size)
            {
                char* buf = new char[size];
                ADT.read(buf, size);
                char* p = buf;
                int t = 0;
                int numStrings = 0;

                // Count the number of strings in the buffer
                while (p < buf + size)
                {
                    numStrings++;
                    p = p + strlen(p) + 1;
                }

                // Allocate the correct number of strings
                ModelInstansName = new std::string[numStrings];
                p = buf;

                for (int i = 0; i < numStrings; ++i)
                {
                   std::string path(p); // Store copy after name fixed
                   std::string uName;
                   ExtractSingleModel(path, uName, failedPaths, iCoreNumber, szRawVMAPMagic, preciseVectorData, szWorkDirWmo);
                   ModelInstansName[i] = uName;
                   p = p + strlen(p) + 1;
                }
                delete[] buf;
            }
        }
        else if (!strcmp(fourcc, "MWMO"))
        {
            // Process MWMO chunk.
            if (size)
            {
                char* buf = new char[size];
                ADT.read(buf, size);
                char* p = buf;
                int q = 0;

                // Count the number of strings in the buffer
                int numStrings = 0;
                while (p < buf + size)
                {
                    numStrings++;
                    p = p + strlen(p) + 1;
                }

                // Allocate the correct number of strings
                WmoInstansName = new std::string[numStrings];
                p = buf;

                while (p < buf + size)
                {
                    std::string path(p);
                    if (q < nWMO)
                    {
                        WmoInstansName[q++] = GetUniformName(path);
                    }
                    else
                    {
                        printf("Error: Attempt to write beyond allocated WmoInstansName array.\n");
                    }
                    p = p + strlen(p) + 1;
                }
                delete[] buf;
            }
        }
        //======================
        else if (!strcmp(fourcc, "MDDF"))
        {
            // Process MDDF chunk.
            if (size)
            {
                nMDX = (int)size / 36;
                for (int i = 0; i < nMDX; ++i)
                {
                    uint32 id;
                    ADT.read(&id, 4);
                    ModelInstance inst(ADT, ModelInstansName[id], map_num, tileX, tileY, dirfile, iCoreNumber, szWorkDirWmo);
                }
                delete[] ModelInstansName;
            }
        }
        else if (!strcmp(fourcc, "MODF"))
        {
            // Process MODF chunk.
            if (size)
            {
                nWMO = (int)size / 64;
                for (int i = 0; i < nWMO; ++i)
                {
                    uint32 id;
                    ADT.read(&id, 4);
                    WMOInstance inst(ADT, WmoInstansName[id], map_num, tileX, tileY, dirfile, szWorkDirWmo);
                }
                delete[] WmoInstansName;
            }
        }
        //======================
        ADT.seek(nextpos);
    }
    ADT.close();
    fclose(dirfile);

    return true;
}

bool ADTFile::initLoadFileFromDisk(uint32 map_num, uint32 tileX, uint32 tileY, StringSet& failedPaths, int iCoreNumber, std::string szRawVMAPMagic, bool preciseVectorData, std::string szWorkDirWmo, std::string localAdtFilename)
{

    FILE* file = fopen(localAdtFilename.c_str(), "rb");
    if (!file)
    {
        //printf("No such file %s\n", localAdtFilename.c_str());
        return false;
    }
    MPQFile ADT(file, localAdtFilename.c_str());
    //if (ADT.isEof())
    //{
    //    return false;
    //}

    uint32 size;

    std::string xMap;
    std::string yMap;
    std::string filename = AdtFilename;

    // Remove the ".adt" extension from the filename.
    filename.erase(filename.find(".adt"), 4);
    std::string TempMapNumber;

    // Extract the map coordinates from the filename.
    TempMapNumber = filename.substr(filename.length() - 6, 6);
    xMap = TempMapNumber.substr(TempMapNumber.find("_") + 1, (TempMapNumber.find_last_of("_") - 1) - (TempMapNumber.find("_")));
    yMap = TempMapNumber.substr(TempMapNumber.find_last_of("_") + 1, (TempMapNumber.length()) - (TempMapNumber.find_last_of("_")));
    filename.erase((filename.length() - xMap.length() - yMap.length() - 2), (xMap.length() + yMap.length() + 2));

    std::string AdtMapNumber = xMap + ' ' + yMap + ' ' + GetUniformName(filename);

    // Open the directory file for writing.
    std::string dirname = std::string(szWorkDirWmo) + "/dir_bin";
    FILE* dirfile;
    dirfile = fopen(dirname.c_str(), "ab");
    if (!dirfile)
    {
        printf("Can't open dirfile!'%s'\n", dirname.c_str());
        return false;
    }

    // Read the ADT file and process its chunks.
    while (!ADT.isEof())
    {
        char fourcc[5];
        ADT.read(&fourcc, 4);
        ADT.read(&size, 4);
        flipcc(fourcc);
        fourcc[4] = 0;

        size_t nextpos = ADT.getPos() + size;

        if (!strcmp(fourcc, "MCIN"))
        {
            // Process MCIN chunk.
        }
        else if (!strcmp(fourcc, "MTEX"))
        {
            // Process MTEX chunk.
        }
        else if (!strcmp(fourcc, "MMDX"))
        {
            // Process MMDX chunk.
            if (size)
            {
                char* buf = new char[size];
                ADT.read(buf, size);
                char* p = buf;
                int t = 0;
                int numStrings = 0;

                // Count the number of strings in the buffer
                while (p < buf + size)
                {
                    numStrings++;
                    p = p + strlen(p) + 1;
                }

                // Allocate the correct number of strings
                ModelInstansName = new std::string[numStrings];
                p = buf;

                for (int i = 0; i < numStrings; ++i)
                {
                   std::string path(p); // Store copy after name fixed
                   std::string uName;
                   ExtractSingleModel(path, uName, failedPaths, iCoreNumber, szRawVMAPMagic, preciseVectorData, szWorkDirWmo);
                   ModelInstansName[i] = uName;
                   p = p + strlen(p) + 1;
                }
                delete[] buf;
            }
        }
        else if (!strcmp(fourcc, "MWMO"))
        {
            // Process MWMO chunk.
            if (size)
            {
                char* buf = new char[size];
                ADT.read(buf, size);
                char* p = buf;
                int q = 0;

                // Count the number of strings in the buffer
                int numStrings = 0;
                while (p < buf + size)
                {
                    numStrings++;
                    p = p + strlen(p) + 1;
                }

                // Allocate the correct number of strings
                WmoInstansName = new std::string[numStrings];
                p = buf;

                while (p < buf + size)
                {
                    std::string path(p);
                    if (q < nWMO)
                    {
                        WmoInstansName[q++] = GetUniformName(path);
                    }
                    else
                    {
                        printf("Error: Attempt to write beyond allocated WmoInstansName array.\n");
                    }
                    p = p + strlen(p) + 1;
                }
                delete[] buf;
            }
        }
        //======================
        else if (!strcmp(fourcc, "MDDF"))
        {
            // Process MDDF chunk.
            if (size)
            {
                nMDX = (int)size / 36;
                for (int i = 0; i < nMDX; ++i)
                {
                    uint32 id;
                    ADT.read(&id, 4);
                    ModelInstance inst(ADT, ModelInstansName[id], map_num, tileX, tileY, dirfile, iCoreNumber, szWorkDirWmo);
                }
                delete[] ModelInstansName;
            }
        }
        else if (!strcmp(fourcc, "MODF"))
        {
            // Process MODF chunk.
            if (size)
            {
                nWMO = (int)size / 64;
                for (int i = 0; i < nWMO; ++i)
                {
                    uint32 id;
                    ADT.read(&id, 4);
                    WMOInstance inst(ADT, WmoInstansName[id], map_num, tileX, tileY, dirfile, szWorkDirWmo);
                }
                delete[] WmoInstansName;
            }
        }
        //======================
        ADT.seek(nextpos);
    }
    ADT.close();
    fclose(dirfile);

    return true;
}

/**
 * @brief Destroys the ADTFile object.
 */
ADTFile::~ADTFile()
{
}

