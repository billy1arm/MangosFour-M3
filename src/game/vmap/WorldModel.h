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

#ifndef MANGOS_H_WORLDMODEL
#define MANGOS_H_WORLDMODEL

#include <G3D/HashTrait.h>
#include <G3D/Vector3.h>
#include <G3D/AABox.h>
#include <G3D/Ray.h>
#include "BIH.h"
#include "Platform/Define.h"

namespace VMAP
{
    class TreeNode;
    struct AreaInfo;
    struct LocationInfo;

    /**
     * @brief Represents a triangle in a mesh.
     */
    class MeshTriangle
    {
    public:
        /**
         * @brief Default constructor initializing indices to 0.
         */
        MeshTriangle() : idx0(0), idx1(0), idx2(0) {};

        /**
         * @brief Constructor initializing indices with given values.
         * @param na Index 0
         * @param nb Index 1
         * @param nc Index 2
         */
        MeshTriangle(uint32 na, uint32 nb, uint32 nc) : idx0(na), idx1(nb), idx2(nc) {};

        uint32 idx0; /**< Index 0 of the triangle */
        uint32 idx1; /**< Index 1 of the triangle */
        uint32 idx2; /**< Index 2 of the triangle */
    };

    /**
     * @brief Represents liquid data in a WMO (World Map Object).
     */
    class WmoLiquid
    {
    public:
        /**
         * @brief Constructor initializing liquid with given dimensions, corner position, and type.
         * @param width Width of the liquid
         * @param height Height of the liquid
         * @param corner Position of the lower corner
         * @param type Type of the liquid
         */
        WmoLiquid(uint32 width, uint32 height, const Vector3& corner, uint32 type);

        /**
         * @brief Copy constructor.
         * @param other Another WmoLiquid object to copy from
         */
        WmoLiquid(const WmoLiquid& other);

        /**
         * @brief Destructor.
         */
        ~WmoLiquid();

        /**
         * @brief Assignment operator.
         * @param other Another WmoLiquid object to assign from
         * @return Reference to this object
         */
        WmoLiquid& operator=(const WmoLiquid& other);

        /**
         * @brief Gets the height of the liquid at a given position.
         * @param pos Position to check
         * @param liqHeight Output parameter for the liquid height
         * @return True if the height was successfully retrieved, false otherwise
         */
        bool GetLiquidHeight(const Vector3& pos, float& liqHeight) const;

        /**
         * @brief Gets the type of the liquid.
         * @return Liquid type
         */
        uint32 GetType() const { return iType; }

        /**
         * @brief Gets the height storage array.
         * @return Pointer to the height storage array
         */
        float* GetHeightStorage() { return iHeight; }

        /**
         * @brief Gets the flags storage array.
         * @return Pointer to the flags storage array
         */
        uint8* GetFlagsStorage() { return iFlags; }

        /**
         * @brief Gets the file size of the liquid data.
         * @return File size in bytes
         */
        uint32 GetFileSize();

        /**
         * @brief Writes the liquid data to a file.
         * @param wf File pointer to write to
         * @return True if the write was successful, false otherwise
         */
        bool WriteToFile(FILE* wf);

        /**
         * @brief Reads the liquid data from a file.
         * @param rf File pointer to read from
         * @param liquid Output parameter for the read liquid data
         * @return True if the read was successful, false otherwise
         */
        static bool ReadFromFile(FILE* rf, WmoLiquid*& liquid);

    private:
        /**
         * @brief Default constructor initializing members to 0.
         */
        WmoLiquid() : iTilesX(0), iTilesY(0), iType(0), iHeight(0), iFlags(0) {};

        uint32 iTilesX;  /**< Number of tiles in x direction */
        uint32 iTilesY;  /**< Number of tiles in y direction */
        Vector3 iCorner; /**< Position of the lower corner */
        uint32 iType;    /**< Type of the liquid */
        float* iHeight;  /**< Height values array */
        uint8* iFlags;   /**< Flags array indicating if liquid tile is used */

#ifdef MMAP_GENERATOR
    public:
        /**
         * @brief Gets the position information of the liquid.
         * @param tilesX Output parameter for the number of tiles in x direction
         * @param tilesY Output parameter for the number of tiles in y direction
         * @param corner Output parameter for the position of the lower corner
         */
        void getPosInfo(uint32& tilesX, uint32& tilesY, Vector3& corner) const;
#endif
    };

    /**
     * @brief Holds additional information for WMO group files.
     */
    class GroupModel
    {
    public:
        /**
         * @brief Default constructor initializing members to 0.
         */
        GroupModel() : iMogpFlags(0), iGroupWMOID(0), iLiquid(0) {}

        /**
         * @brief Copy constructor.
         * @param other Another GroupModel object to copy from
         */
        GroupModel(const GroupModel& other);

        /**
         * @brief Constructor initializing with given flags, group WMO ID, and bounding box.
         * @param mogpFlags Flags for the group model
         * @param groupWMOID Group WMO ID
         * @param bound Bounding box of the group model
         */
        GroupModel(uint32 mogpFlags, uint32 groupWMOID, const AABox& bound) :
                iBound(bound), iMogpFlags(mogpFlags), iGroupWMOID(groupWMOID), iLiquid(0) {}

        /**
         * @brief Destructor.
         */
        ~GroupModel() { delete iLiquid; }

        /**
         * @brief Sets the mesh data for the group model.
         * @param vert Vector of vertices
         * @param tri Vector of triangles
         */
        void SetMeshData(std::vector<Vector3>& vert, std::vector<MeshTriangle>& tri);

        /**
         * @brief Sets the liquid data for the group model.
         * @param liquid Pointer to the liquid data
         */
        void setLiquidData(WmoLiquid*& liquid) { iLiquid = liquid; liquid = NULL; }

        /**
         * @brief Checks if a ray intersects with the group model.
         * @param ray Ray to check
         * @param distance Output parameter for the distance to the intersection
         * @param stopAtFirstHit Whether to stop at the first hit
         * @return True if the ray intersects, false otherwise
         */
        bool IntersectRay(const G3D::Ray& ray, float& distance, bool stopAtFirstHit) const;

        /**
         * @brief Checks if a position is inside the group model.
         * @param pos Position to check
         * @param down Direction vector pointing downwards
         * @param z_dist Output parameter for the distance to the ground
         * @return True if the position is inside, false otherwise
         */
        bool IsInsideObject(const Vector3& pos, const Vector3& down, float& z_dist) const;

        /**
         * @brief Gets the liquid level at a given position.
         * @param pos Position to check
         * @param liqHeight Output parameter for the liquid height
         * @return True if the liquid level was successfully retrieved, false otherwise
         */
        bool GetLiquidLevel(const Vector3& pos, float& liqHeight) const;

        /**
         * @brief Gets the type of the liquid in the group model.
         * @return Liquid type
         */
        uint32 GetLiquidType() const;

        /**
         * @brief Writes the group model data to a file.
         * @param wf File pointer to write to
         * @return True if the write was successful, false otherwise
         */
        bool WriteToFile(FILE* wf);

        /**
         * @brief Reads the group model data from a file.
         * @param rf File pointer to read from
         * @return True if the read was successful, false otherwise
         */
        bool ReadFromFile(FILE* rf);

        /**
         * @brief Gets the bounding box of the group model.
         * @return Bounding box
         */
        const G3D::AABox& GetBound() const { return iBound; }

        /**
         * @brief Gets the flags of the group model.
         * @return Flags
         */
        uint32 GetMogpFlags() const { return iMogpFlags; }

        /**
         * @brief Gets the group WMO ID.
         * @return Group WMO ID
         */
        uint32 GetWmoID() const { return iGroupWMOID; }

    protected:
        G3D::AABox iBound;  /**< Bounding box of the group model */
        uint32 iMogpFlags;  /**< Flags for the group model */
        uint32 iGroupWMOID; /**< Group WMO ID */
        std::vector<Vector3> vertices; /**< Vertices of the group model */
        std::vector<MeshTriangle> triangles; /**< Triangles of the group model */
        BIH meshTree; /**< Bounding Interval Hierarchy for the mesh */
        WmoLiquid* iLiquid; /**< Liquid data for the group model */

#ifdef MMAP_GENERATOR
    public:
        /**
         * @brief Gets the mesh data of the group model.
         * @param vertices Output parameter for the vertices
         * @param triangles Output parameter for the triangles
         * @param liquid Output parameter for the liquid data
         */
        void getMeshData(std::vector<Vector3>& vertices, std::vector<MeshTriangle>& triangles, WmoLiquid*& liquid);
#endif
    };

    /**
     * @brief Holds a model (converted M2 or WMO) in its original coordinate space.
     */
    class WorldModel
    {
    public:
        /**
         * @brief Default constructor initializing members to 0.
         */
        WorldModel() : RootWMOID(0), Flags(0) {}

        /**
         * @brief Sets the group models for the world model.
         * @param models Vector of group models
         */
        void SetGroupModels(std::vector<GroupModel>& models);

        /**
         * @brief Sets the root WMO ID.
         * @param id Root WMO ID
         */
        void SetRootWmoID(uint32 id) { RootWMOID = id; }

        /**
         * @brief Checks if a ray intersects with the world model.
         * @param ray Ray to check
         * @param distance Output parameter for the distance to the intersection
         * @param stopAtFirstHit Whether to stop at the first hit
         * @return True if the ray intersects, false otherwise
         */
        bool IntersectRay(const G3D::Ray& ray, float& distance, bool stopAtFirstHit) const;

        /**
         * @brief Gets area information at a given position.
         * @param p Position to check
         * @param down Direction vector pointing downwards
         * @param dist Output parameter for the distance to the ground
         * @param info Output parameter for the area information
         * @return True if the area information was successfully retrieved, false otherwise
         */
        bool GetAreaInfo(const G3D::Vector3& p, const G3D::Vector3& down, float& dist, AreaInfo& info) const;

        /**
         * @brief Gets location information at a given position.
         * @param p Position to check
         * @param down Direction vector pointing downwards
         * @param dist Output parameter for the distance to the ground
         * @param info Output parameter for the location information
         * @return True if the location information was successfully retrieved, false otherwise
         */
        bool GetLocationInfo(const G3D::Vector3& p, const G3D::Vector3& down, float& dist, LocationInfo& info) const;

        /**
         * @brief Writes the world model data to a file.
         * @param filename Name of the file to write to
         * @return True if the write was successful, false otherwise
         */
        bool writeFile(const std::string& filename);

        /**
         * @brief Reads the world model data from a file.
         * @param filename Name of the file to read from
         * @return True if the read was successful, false otherwise
         */
        bool readFile(const std::string& filename);

        uint32 Flags; /**< Flags for the world model */

    protected:
        uint32 RootWMOID; /**< Root WMO ID */
        std::vector<GroupModel> groupModels; /**< Group models in the world model */
        BIH groupTree; /**< Bounding Interval Hierarchy for the group models */

#ifdef MMAP_GENERATOR
    public:
        /**
         * @brief Gets the group models of the world model.
         * @param groupModels Output parameter for the group models
         */
        void getGroupModels(std::vector<GroupModel>& groupModels);
#endif
    };
} // namespace VMAP

#endif // MANGOS_H_WORLDMODEL
