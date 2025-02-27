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

#ifndef MANGOS_H_MAPTREE
#define MANGOS_H_MAPTREE

#include "Platform/Define.h"
#include "BIH.h"

#include <unordered_map>

namespace VMAP
{
    class ModelInstance;
    class GroupModel;
    class VMapManager2;

    /**
     * @brief Structure representing location information.
     *
     */
    struct LocationInfo
    {
        /**
         * @brief Constructor to initialize member variables.
         *
         * Initializes hitInstance and hitModel to nullptr, and ground_Z to negative infinity.
         */
        LocationInfo(): hitInstance(0), hitModel(0), ground_Z(-G3D::inf()) {};

        const ModelInstance* hitInstance; /**< Pointer to the hit model instance. */
        const GroupModel* hitModel; /**< Pointer to the hit group model. */
        float ground_Z; /**< The ground height at the specified location. */
    };

    /**
     * @brief Class representing a static map tree.
     *
     * This class manages the loading and unloading of map tiles, and provides methods for
     * querying information about the map, such as line of sight, object hit positions, and area info.
     */
    class StaticMapTree
    {
            /**
             * @brief Type definition for a map of loaded tiles.
             *
             * Maps tile IDs to a boolean indicating whether the tile is loaded.
             */
            typedef std::unordered_map<uint32, bool> loadedTileMap;
            /**
             * @brief Type definition for a map of loaded spawns.
             *
             * Maps tree indices to reference counts.
             */
            typedef std::unordered_map<uint32, uint32> loadedSpawnMap;
        private:
            uint32 iMapID; /**< The ID of the map. */
            bool iIsTiled; /**< Indicates whether the map is tiled. */
            BIH iTree; /**< Bounding Interval Hierarchy for the map. */
            ModelInstance* iTreeValues; /**< The tree entries (model instances). */
            uint32 iNTreeValues; /**< The number of tree entries. */

            // Store all the map tile idents that are loaded for that map
            // some maps are not splitted into tiles and we have to make sure, not removing the map before all tiles are removed
            // empty tiles have no tile file, hence map with bool instead of just a set (consistency check)
            loadedTileMap iLoadedTiles; /**< Map of loaded tiles. */
            // stores <tree_index, reference_count> to invalidate tree values, unload map, and to be able to report errors
            loadedSpawnMap iLoadedSpawns; /**< Map of loaded spawns. */
            std::string iBasePath; /**< The base path for the map files. */

        private:
            /**
             * @brief Calculates the intersection time of a ray with the map.
             *
             * @param pRay The ray to check for intersection.
             * @param pMaxDist The maximum distance for the intersection.
             * @param pStopAtFirstHit Whether to stop at the first hit.
             * @return True if an intersection is found, false otherwise.
             */
            bool getIntersectionTime(const G3D::Ray& pRay, float& pMaxDist, bool pStopAtFirstHit) const;
            // bool containsLoadedMapTile(unsigned int pTileIdent) const { return(iLoadedMapTiles.containsKey(pTileIdent)); }
        public:
            /**
             * @brief Generates the file name for a map tile.
             *
             * @param mapID The ID of the map.
             * @param tileX The X coordinate of the tile.
             * @param tileY The Y coordinate of the tile.
             * @return The file name of the map tile.
             */
            static std::string getTileFileName(uint32 mapID, uint32 tileX, uint32 tileY);
            /**
             * @brief Packs the tile coordinates into a single ID.
             *
             * @param tileX The X coordinate of the tile.
             * @param tileY The Y coordinate of the tile.
             * @return The packed tile ID.
             */
            static uint32 packTileID(uint32 tileX, uint32 tileY) { return tileX << 16 | tileY; }
            /**
             * @brief Unpacks the tile ID into coordinates.
             *
             * @param ID The packed tile ID.
             * @param tileX The X coordinate of the tile.
             * @param tileY The Y coordinate of the tile.
             */
            static void unpackTileID(uint32 ID, uint32& tileX, uint32& tileY) { tileX = ID >> 16; tileY = ID & 0xFF; }
            /**
             * @brief Checks if a map tile can be loaded.
             *
             * @param basePath The base path for the map files.
             * @param mapID The ID of the map.
             * @param tileX The X coordinate of the tile.
             * @param tileY The Y coordinate of the tile.
             * @return True if the map tile can be loaded, false otherwise.
             */
            static bool CanLoadMap(const std::string& basePath, uint32 mapID, uint32 tileX, uint32 tileY);

            /**
             * @brief Constructor for StaticMapTree.
             *
             * @param mapID The ID of the map.
             * @param basePath The base path for the map files.
             */
            StaticMapTree(uint32 mapID, const std::string& basePath);
            /**
             * @brief Destructor for StaticMapTree.
             *
             */
            ~StaticMapTree();

            /**
             * @brief Checks if there is a line of sight between two positions.
             *
             * @param pos1 The first position.
             * @param pos2 The second position.
             * @return True if there is a line of sight, false otherwise.
             */
            bool isInLineOfSight(const G3D::Vector3& pos1, const G3D::Vector3& pos2) const;
            /**
             * @brief Gets the hit position of an object between two positions.
             *
             * @param pos1 The first position.
             * @param pos2 The second position.
             * @param pResultHitPos The resulting hit position.
             * @param pModifyDist The distance to modify the hit position.
             * @return True if an object is hit, false otherwise.
             */
            bool getObjectHitPos(const G3D::Vector3& pos1, const G3D::Vector3& pos2, G3D::Vector3& pResultHitPos, float pModifyDist) const;
            /**
             * @brief Gets the height at a specified position.
             *
             * @param pPos The position to check.
             * @param maxSearchDist The maximum search distance.
             * @return The height at the specified position.
             */
            float getHeight(const G3D::Vector3& pPos, float maxSearchDist) const;
            /**
             * @brief Gets the area information at a specified position.
             *
             * @param pos The position to check.
             * @param flags The flags indicating properties of the area.
             * @param adtId The ADT (map tile) ID.
             * @param rootId The root WMO (World Map Object) ID.
             * @param groupId The group WMO ID.
             * @return True if the area information is retrieved, false otherwise.
             */
            bool getAreaInfo(G3D::Vector3& pos, uint32& flags, int32& adtId, int32& rootId, int32& groupId) const;
            /**
             * @brief Gets the location information at a specified position.
             *
             * @param pos The position to check.
             * @param info The location information to be filled.
             * @return True if the location information is retrieved, false otherwise.
             */
            bool GetLocationInfo(const Vector3& pos, LocationInfo& info) const;

            /**
             * @brief Initializes the map.
             *
             * @param fname The file name of the map.
             * @param vm The VMapManager2 instance.
             * @return True if the map is initialized, false otherwise.
             */
            bool InitMap(const std::string& fname, VMapManager2* vm);
            /**
             * @brief Unloads the map.
             *
             * @param vm The VMapManager2 instance.
             */
            void UnloadMap(VMapManager2* vm);
            /**
             * @brief Loads a map tile.
             *
             * @param tileX The X coordinate of the tile.
             * @param tileY The Y coordinate of the tile.
             * @param vm The VMapManager2 instance.
             * @return True if the map tile is loaded, false otherwise.
             */
            bool LoadMapTile(uint32 tileX, uint32 tileY, VMapManager2* vm);
            /**
             * @brief Unloads a map tile.
             *
             * @param tileX The X coordinate of the tile.
             * @param tileY The Y coordinate of the tile.
             * @param vm The VMapManager2 instance.
             */
            void UnloadMapTile(uint32 tileX, uint32 tileY, VMapManager2* vm);
            /**
             * @brief Checks if the map is tiled.
             *
             * @return True if the map is tiled, false otherwise.
             */
            bool isTiled() const { return iIsTiled; }
            /**
             * @brief Gets the number of loaded tiles.
             *
             * @return The number of loaded tiles.
             */
            uint32 numLoadedTiles() const { return iLoadedTiles.size(); }

#ifdef MMAP_GENERATOR
        public:
            /**
             * @brief Gets the model instances.
             *
             * @param models The array of model instances to be filled.
             * @param count The number of model instances.
             */
            void getModelInstances(ModelInstance*& models, uint32& count);
#endif
    };

    /**
     * @brief Structure representing area information.
     *
     */
    struct AreaInfo
    {
        /**
         * @brief Constructor to initialize member variables.
         *
         * Initializes the result to false, ground_Z to negative infinity,
         * and flags, adtId, rootId, and groupId to 0.
         */
        AreaInfo(): result(false), ground_Z(-G3D::inf()), flags(0), adtId(0), rootId(0), groupId(0) {};

        bool result; /**< Indicates whether the area information retrieval was successful. */
        float ground_Z; /**< The ground height at the specified location. */
        uint32 flags; /**< Flags indicating properties of the area. */
        int32 adtId; /**< The ADT (map tile) ID. */
        int32 rootId; /**< The root WMO (World Map Object) ID. */
        int32 groupId; /**< The group WMO ID. */
    };
} // namespace VMAP

#endif // MANGOS_H_MAPTREE
