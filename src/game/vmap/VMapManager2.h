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

#ifndef _VMAPMANAGER2_H
#define _VMAPMANAGER2_H

#include "IVMapManager.h"
#include "Platform/Define.h"
#include <G3D/Vector3.h>

#include <unordered_map>

#define MAP_FILENAME_EXTENSION2 ".vmtree"

#define FILENAMEBUFFER_SIZE 500

/**
 * This is the main Class to manage loading and unloading of maps, line of sight, height calculation and so on.
 * For each map or map tile to load it reads a directory file that contains the ModelContainer files used by this map or map tile.
 * Each global map or instance has its own dynamic BSP-Tree.
 * The loaded ModelContainers are included in one of these BSP-Trees.
 * Additionally a table to match map ids and map names is used.
 */

namespace VMAP
{
    class StaticMapTree;
    class WorldModel;

    /**
     * @brief Class to manage a model with reference counting.
     */
    class ManagedModel
    {
    public:
        /**
         * @brief Default constructor initializing members to 0.
         */
        ManagedModel() : iModel(0), iRefCount(0) {}

        /**
         * @brief Sets the model.
         * @param model Pointer to the WorldModel
         */
        void setModel(WorldModel* model) { iModel = model; }

        /**
         * @brief Gets the model.
         * @return Pointer to the WorldModel
         */
        WorldModel* getModel() { return iModel; }

        /**
         * @brief Increments the reference count.
         */
        void incRefCount() { ++iRefCount; }

        /**
         * @brief Decrements the reference count.
         * @return The new reference count
         */
        int decRefCount() { return --iRefCount; }

    protected:
        WorldModel* iModel; /**< Pointer to the WorldModel */
        int iRefCount; /**< Reference count */
    };

    /**
     * @brief Map of instance trees.
     */
    typedef std::unordered_map<uint32, StaticMapTree*> InstanceTreeMap;

    /**
     * @brief Map of managed models.
     */
    typedef std::unordered_map<std::string, ManagedModel> ModelFileMap;

    /**
     * @brief Enumeration for disable types.
     */
    enum DisableTypes
    {
        VMAP_DISABLE_AREAFLAG     = 0x1,
        VMAP_DISABLE_HEIGHT       = 0x2,
        VMAP_DISABLE_LOS          = 0x4,
        VMAP_DISABLE_LIQUIDSTATUS = 0x8
    };

    /**
     * @brief Class to manage VMAPs.
     */
    class VMapManager2 : public IVMapManager
    {
    protected:
        ModelFileMap iLoadedModelFiles; /**< Map of loaded model files */
        InstanceTreeMap iInstanceMapTrees; /**< Map of instance trees */

        /**
         * @brief Loads a map tile.
         * @param pMapId Map ID
         * @param basePath Base path to the map files
         * @param tileX Tile X coordinate
         * @param tileY Tile Y coordinate
         * @return True if the map tile was loaded successfully, false otherwise
         */
        bool _loadMap(uint32 pMapId, const std::string& basePath, uint32 tileX, uint32 tileY);

    public:
        /**
         * @brief Converts a position to internal representation.
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         * @return Converted position
         */
        G3D::Vector3 convertPositionToInternalRep(float x, float y, float z) const;

        /**
         * @brief Gets the map file name for a given map ID.
         * @param pMapId Map ID
         * @return Map file name
         */
        static std::string getMapFileName(unsigned int pMapId);

        /**
         * @brief Constructor for VMapManager2.
         */
        VMapManager2();

        /**
         * @brief Destructor for VMapManager2.
         */
        ~VMapManager2();

        /**
         * @brief Loads a map tile.
         * @param pBasePath Base path to the map files
         * @param pMapId Map ID
         * @param x Tile X coordinate
         * @param y Tile Y coordinate
         * @return VMAPLoadResult indicating the result of the load operation
         */
        VMAPLoadResult loadMap(const char* pBasePath, unsigned int pMapId, int x, int y) override;

        /**
         * @brief Unloads a map tile.
         * @param pMapId Map ID
         * @param x Tile X coordinate
         * @param y Tile Y coordinate
         */
        void unloadMap(unsigned int pMapId, int x, int y) override;

        /**
         * @brief Unloads a map.
         * @param pMapId Map ID
         */
        void unloadMap(unsigned int pMapId) override;

        /**
         * @brief Checks if there is a line of sight between two points.
         * @param pMapId Map ID
         * @param x1 X coordinate of the first point
         * @param y1 Y coordinate of the first point
         * @param z1 Z coordinate of the first point
         * @param x2 X coordinate of the second point
         * @param y2 Y coordinate of the second point
         * @param z2 Z coordinate of the second point
         * @return True if there is a line of sight, false otherwise
         */
        bool isInLineOfSight(unsigned int pMapId, float x1, float y1, float z1, float x2, float y2, float z2) override;

        /**
         * @brief Gets the hit position of an object between two points.
         * @param pMapId Map ID
         * @param x1 X coordinate of the first point
         * @param y1 Y coordinate of the first point
         * @param z1 Z coordinate of the first point
         * @param x2 X coordinate of the second point
         * @param y2 Y coordinate of the second point
         * @param z2 Z coordinate of the second point
         * @param rx Output parameter for the X coordinate of the hit position
         * @param ry Output parameter for the Y coordinate of the hit position
         * @param rz Output parameter for the Z coordinate of the hit position
         * @param pModifyDist Distance to modify the hit position
         * @return True if an object was hit, false otherwise
         */
        bool getObjectHitPos(unsigned int pMapId, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float& ry, float& rz, float pModifyDist) override;

        /**
         * @brief Gets the height at a given position.
         * @param pMapId Map ID
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         * @param maxSearchDist Maximum search distance
         * @return Height at the given position
         */
        float getHeight(unsigned int pMapId, float x, float y, float z, float maxSearchDist) override;

        /**
         * @brief Processes a command (for debug and extensions).
         * @param pCommand Command to process
         * @return False (not implemented)
         */
        bool processCommand(char* pCommand) override { return false; }

        /**
         * @brief Gets area information at a given position.
         * @param pMapId Map ID
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Output parameter for the Z coordinate (ground height)
         * @param flags Output parameter for the area flags
         * @param adtId Output parameter for the ADT ID
         * @param rootId Output parameter for the root WMO ID
         * @param groupId Output parameter for the group WMO ID
         * @return True if the area information was successfully retrieved, false otherwise
         */
        bool getAreaInfo(unsigned int pMapId, float x, float y, float& z, uint32& flags, int32& adtId, int32& rootId, int32& groupId) const override;

        /**
         * @brief Gets the liquid level at a given position.
         * @param pMapId Map ID
         * @param x X coordinate
         * @param y Y coordinate
         * @param z Z coordinate
         * @param ReqLiquidType Required liquid type
         * @param level Output parameter for the liquid level
         * @param floor Output parameter for the liquid floor
         * @param type Output parameter for the liquid type
         * @return True if the liquid level was successfully retrieved, false otherwise
         */
        bool GetLiquidLevel(uint32 pMapId, float x, float y, float z, uint8 ReqLiquidType, float& level, float& floor, uint32& type) const override;

        /**
         * @brief Acquires a model instance.
         * @param basepath Base path to the model files
         * @param filename Name of the model file
         * @return Pointer to the WorldModel
         */
        WorldModel* acquireModelInstance(const std::string& basepath, const std::string& filename);

        /**
         * @brief Releases a model instance.
         * @param filename Name of the model file
         */
        void releaseModelInstance(const std::string& filename);

        /**
         * @brief Gets the directory file name for a given map ID and tile coordinates.
         * @param pMapId Map ID
         * @param x Tile X coordinate
         * @param y Tile Y coordinate
         * @return Directory file name
         */
        std::string getDirFileName(unsigned int pMapId, int x, int y) const override
        {
            return getMapFileName(pMapId);
        }

        /**
         * @brief Checks if a map tile exists.
         * @param pBasePath Base path to the map files
         * @param pMapId Map ID
         * @param x Tile X coordinate
         * @param y Tile Y coordinate
         * @return True if the map tile exists, false otherwise
         */
        bool existsMap(const char* pBasePath, unsigned int pMapId, int x, int y) override;

        typedef bool(*IsVMAPDisabledForFn)(uint32 entry, uint8 flags);
        IsVMAPDisabledForFn IsVMAPDisabledForPtr;

#ifdef MMAP_GENERATOR
    public:
        /**
         * @brief Gets the instance map tree.
         * @param instanceMapTree Output parameter for the instance map tree
         */
        void getInstanceMapTree(InstanceTreeMap& instanceMapTree);
#endif
    };
}
#endif // _VMAPMANAGER2_H
