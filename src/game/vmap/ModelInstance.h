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

#ifndef MANGOS_H_MODELINSTANCE
#define MANGOS_H_MODELINSTANCE

#include <G3D/Matrix3.h>
#include <G3D/Vector3.h>
#include <G3D/AABox.h>
#include <G3D/Ray.h>

#include "Platform/Define.h"

namespace VMAP
{
    class WorldModel;
    struct AreaInfo;
    struct LocationInfo;

    /**
     * @brief Enumeration for model flags.
     */
    enum ModelFlags
    {
        MOD_M2 = 1,
        MOD_WORLDSPAWN = 1 << 1,
        MOD_HAS_BOUND = 1 << 2
    };

    /**
     * @brief Class representing a model spawn.
     */
    class ModelSpawn
    {
        public:
            uint32 flags = 0; /**< Flags for the model spawn. */
            uint16 adtId = 0; /**< ADT ID for the model spawn. */
            uint32 ID = 0; /**< ID for the model spawn. */
            G3D::Vector3 iPos; /**< Position of the model spawn. */
            G3D::Vector3 iRot; /**< Rotation of the model spawn. */
            float iScale = 1.0f; /**< Scale of the model spawn. */
            G3D::AABox iBound; /**< Bounding box of the model spawn. */
            std::string name; /**< Name of the model spawn. */

            /**
             * @brief Equality operator for ModelSpawn.
             *
             * @param other The other ModelSpawn to compare with.
             * @return True if the IDs are equal, false otherwise.
             */
            bool operator==(const ModelSpawn& other) const { return ID == other.ID; }

            /**
             * @brief Get the bounding box of the model spawn.
             *
             * @return The bounding box of the model spawn.
             */
            const G3D::AABox& getBounds() const { return iBound; }

            /**
             * @brief Read a ModelSpawn from a file.
             *
             * @param rf The file to read from.
             * @param spawn The ModelSpawn to read into.
             * @return True if successful, false otherwise.
             */
            static bool ReadFromFile(FILE* rf, ModelSpawn& spawn);

            /**
             * @brief Write a ModelSpawn to a file.
             *
             * @param rw The file to write to.
             * @param spawn The ModelSpawn to write.
             * @return True if successful, false otherwise.
             */
            static bool WriteToFile(FILE* rw, const ModelSpawn& spawn);
    };

    /**
     * @brief Class representing a model instance.
     */
    class ModelInstance: public ModelSpawn
    {
        public:
            /**
             * @brief Default constructor for ModelInstance.
             */
            ModelInstance(): iInvScale(1.0f), iModel(0) {}

            /**
             * @brief Constructor for ModelInstance.
             *
             * @param spawn The ModelSpawn to initialize from.
             * @param model The WorldModel associated with this instance.
             */
            ModelInstance(const ModelSpawn& spawn, WorldModel* model);

            /**
             * @brief Set the model instance as unloaded.
             */
            void setUnloaded() { iModel = 0; }

            /**
             * @brief Check if a ray intersects with the model instance.
             *
             * @param pRay The ray to check.
             * @param pMaxDist The maximum distance for the intersection.
             * @param pStopAtFirstHit Whether to stop at the first hit.
             * @return True if the ray intersects, false otherwise.
             */
            bool IntersectRay(const G3D::Ray& pRay, float& pMaxDist, bool pStopAtFirstHit) const;

            /**
             * @brief Check if a point intersects with the model instance.
             *
             * @param p The point to check.
             * @param info The area info to update.
             */
            void intersectPoint(const G3D::Vector3& p, AreaInfo& info) const;

            /**
             * @brief Get the location info for a point.
             *
             * @param p The point to check.
             * @param info The location info to update.
             * @return True if successful, false otherwise.
             */
            bool GetLocationInfo(const G3D::Vector3& p, LocationInfo& info) const;

            /**
             * @brief Get the liquid level for a point.
             *
             * @param p The point to check.
             * @param info The location info to update.
             * @param liqHeight The liquid height to update.
             * @return True if successful, false otherwise.
             */
            bool GetLiquidLevel(const G3D::Vector3& p, LocationInfo& info, float& liqHeight) const;

        protected:
            G3D::Matrix3 iInvRot; /**< Inverse rotation matrix for the model instance. */
            float iInvScale; /**< Inverse scale for the model instance. */
            WorldModel* iModel; /**< Pointer to the WorldModel associated with this instance. */

#ifdef MMAP_GENERATOR
        public:
            /**
             * @brief Get the WorldModel associated with this instance.
             *
             * @return The WorldModel associated with this instance.
             */
            WorldModel* const getWorldModel();
#endif
    };
} // namespace VMAP

#endif // MANGOS_H_MODELINSTANCE
