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

/// \addtogroup realmd
/// @{
/// \file

#ifndef MANGOS_H_REALMLIST
#define MANGOS_H_REALMLIST

#include <ace/Singleton.h>
#include <ace/Null_Mutex.h>
#include <ace/INET_Addr.h>
#include "Common.h"

/**
 * @brief Structure to hold information about a specific build of the game client.
 *
 */
struct RealmBuildInfo
{
    int build; /**< Build number */
    int major_version; /**< Major version number */
    int minor_version; /**< Minor version number */
    int bugfix_version; /**< Bugfix version number */
    int hotfix_version; /**< Hotfix version identifier */
};

/**
 * @brief Enumeration of the different versions of the game client.
 *
 */
enum RealmVersion
{
    REALM_VERSION_VANILLA     = 0,
    REALM_VERSION_TBC         = 1,
    REALM_VERSION_WOTLK       = 2,
    REALM_VERSION_CATA        = 3,
    REALM_VERSION_MOP         = 4,
    REALM_VERSION_WOD         = 5,
    REALM_VERSION_LEGION      = 6,
    REALM_VERSION_BFA         = 7,
    REALM_VERSION_SHADOWLANDS = 8,
    REALM_VERSION_COUNT       = 9
};

/**
 * This is used to make a link between build number and actual wow version that
 * it belongs to. To get the connection between them, ie turn a build into a version
 * one would use \ref RealmList::BelongsToVersion the other way around is not available
 * as it does not make sense and isn't needed.
 */
RealmBuildInfo const* FindBuildInfo(uint16 _build);

/**
 * @brief Set of build numbers supported by a realm.
 *
 */
typedef std::set<uint32> RealmBuilds;

/// Storage object for a realm
/**
 * @brief Structure to hold information about a realm.
 *
 */
struct Realm
{
    std::string name; /**< Name of the realm */
    ACE_INET_Addr ExternalAddress; /**< External IP address of the realm */
    ACE_INET_Addr LocalAddress; /**< Local IP address of the realm */
    ACE_INET_Addr LocalSubnetMask; /**< Subnet mask for the local network */
    uint8 icon = 0; /**< Icon representing the realm */
    RealmFlags realmflags = RealmFlags(0); /**< Flags representing the state of the realm */
    uint8 timezone = 0; /**< Timezone of the realm */
    uint32 m_ID = 0; /**< Unique ID of the realm */
    AccountTypes allowedSecurityLevel = SEC_PLAYER; /**< Current allowed join security level (show as locked for not fit accounts) */
    float populationLevel = 0.0f; /**< Population level of the realm */
    RealmBuilds realmbuilds; /**< List of supported builds (updated in DB by mangosd) */
    RealmBuildInfo realmBuildInfo = {0, 0, 0, 0, ' '}; /**< Build info for show version in list */
};

/**
 * @brief Storage object for the list of realms on the server.
 *
 */
class RealmList
{
    public:
        /**
         * @brief Map of realm names to Realm objects.
         *
         */
        typedef std::map<std::string, Realm> RealmMap;
        typedef std::list<const Realm*> RealmStlList;
        typedef std::pair<RealmStlList::const_iterator, RealmStlList::const_iterator> RealmListIterators;
        typedef std::map<uint32, RealmVersion> RealmBuildVersionMap;

        /**
         * @brief Get the singleton instance of the RealmList.
         *
         * @return RealmList& Reference to the singleton instance.
         */
        static RealmList& Instance();

        /**
         * @brief Constructor for RealmList.
         *
         */
        RealmList();

        /**
         * @brief Destructor for RealmList.
         *
         */
        ~RealmList() {};

        /**
         * @brief Initialize the realm list with the specified update interval.
         *
         * @param updateInterval Interval in seconds between updates.
         */
        void Initialize(uint32 updateInterval);

        /**
         * @brief Initializes a map holding a link from build number to a version.
         * \see RealmVersion
         */
        void InitVersionToBuild();

        /**
         * @brief Update the realm list if needed.
         *
         */
        void UpdateIfNeed();

        /**
         * @brief Get the iterators for all realms supporting the given version as a pair.
         *
         * The first member is an iterator to the begin() and the second is an iterator
         * to the end().
         *
         * @param build The build number to fetch the iterators for.
         * @return RealmListIterators Iterators to the begin() and end() part of the realms supporting
         * the given build. If there is no matching build, iterators are given to end() and end() of a list.
         */
        RealmListIterators GetIteratorsForBuild(uint32 build) const;

        /**
         * @brief Returns how many realms are available for the current build.
         *
         * @param build The build we want to know the number of available realms for.
         * @return uint32 The number of available realms.
         */
        uint32 NumRealmsForBuild(uint32 build) const;

        /**
         * @brief Returns the total number of realms available.
         *
         * @return uint32 The total number of realms.
         * \see RealmList::NumRealmsForBuild
         */
        uint32 size() const { return m_realms.size(); };
    private:
        /**
         * @brief Checks what version (ie, vanilla, tbc) a certain build number belongs to.
         *
         * @param build The build you want to check the version for.
         * @return RealmVersion The corresponding version to the given build number.
         */
        RealmVersion BelongsToVersion(uint32 build) const;

        /**
         * @brief Adds entries to a map containing a link from a build number to a certain
         * wow version, ie: \ref RealmVersion::REALM_VERSION_VANILLA.
         * \see RealmVersion
         */
        void InitBuildToVersion();

        /**
         * @brief Adds the given \ref Realm to a list sorted by version, ie: vanilla, tbc etc.
         *
         * This in turn is used to only present the compatible realms to the clients connecting,
         * ie: vanilla clients will only see vanilla realms.
         *
         * This is controlled by what you set in the allowedbuilds field in the realm.realmlist
         * database. If you set more than one build, the first one found in there will be
         * used. So if you tag a realm as this: "8606 6141" only TBC clients will be able to
         * see the realm and connect to it.
         *
         * @param realm The realm you want to add to the sorted list. Should be done for all realms.
         * \see RealmVersion
         */
        void AddRealmToBuildList(const Realm& realm);

        /**
         * @brief Update the realms from the database.
         *
         * @param init Whether this is the initial update.
         */
        void UpdateRealms(bool init);

        /**
         * @brief Update or create a new realm entry.
         *
         * @param ID The ID of the realm.
         * @param name The name of the realm.
         * @param address The external address of the realm.
         * @param localAddress The local address of the realm.
         * @param localSubnetmask The local subnet mask of the realm.
         * @param port The port of the realm.
         * @param icon The icon representing the realm.
         * @param realmflags The flags representing the state of the realm.
         * @param timezone The timezone of the realm.
         * @param allowedSecurityLevel The allowed security level for the realm.
         * @param popu The population level of the realm.
         * @param builds The builds supported by the realm.
         */
        void UpdateRealm(uint32 ID, const std::string& name, ACE_INET_Addr const& address, ACE_INET_Addr const& localAddress, ACE_INET_Addr const& localSubnetmask, uint32 port, uint8 icon, RealmFlags realmflags, uint8 timezone, AccountTypes allowedSecurityLevel, float popu, const std::string& builds);
    private:
        RealmMap m_realms;                                    ///< Internal map of realms
        RealmStlList m_realmsByVersion[REALM_VERSION_COUNT]; ///< This sorts the realms by their supported build
        RealmBuildVersionMap m_buildToVersion;               ///< Map linking build numbers to versions
        uint32   m_UpdateInterval;                           ///< Interval in seconds between updates
        time_t   m_NextUpdateTime;                           ///< Time of the next update
};

#define sRealmList RealmList::Instance()

#endif
/// @}