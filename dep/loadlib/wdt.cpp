//#define _CRT_SECURE_NO_DEPRECATE

#include "wdt.h"

/**
 * @brief Prepare loaded data for wdt_MWMO.
 * @return True if the data is prepared successfully, false otherwise.
 */
bool wdt_MWMO::prepareLoadedData() const
{
    if (fcc != 'MWMO')
        return false;
    return true;
}

/**
 * @brief Prepare loaded data for wdt_MPHD.
 * @return True if the data is prepared successfully, false otherwise.
 */
bool wdt_MPHD::prepareLoadedData() const
{
    if (fcc != 'MPHD')
        return false;
    return true;
}

/**
 * @brief Prepare loaded data for wdt_MAIN.
 * @return True if the data is prepared successfully, false otherwise.
 */
bool wdt_MAIN::prepareLoadedData() const
{
    if (fcc != 'MAIN')
        return false;
    return true;
}

/**
 * @brief Constructor for WDT_file.
 */
WDT_file::WDT_file()
{
    mphd = 0;
    main = 0;
    wmo  = 0;
}

/**
 * @brief Destructor for WDT_file.
 */
WDT_file::~WDT_file()
{
    free();
}

/**
 * @brief Free the resources used by WDT_file.
 */
void WDT_file::free()
{
    mphd = 0;
    main = 0;
    wmo  = 0;
    FileLoader::free();
}

/**
 * @brief Prepare loaded data for WDT_file.
 * @return True if the data is prepared successfully, false otherwise.
 */
bool WDT_file::prepareLoadedData()
{
    // Check parent
    if (!FileLoader::prepareLoadedData())
        return false;

    mphd = (wdt_MPHD*)((uint8*)version + version->size + 8);
    if (!mphd->prepareLoadedData())
        return false;
    main = (wdt_MAIN*)((uint8*)mphd + mphd->size + 8);
    if (!main->prepareLoadedData())
        return false;
    wmo = (wdt_MWMO*)((uint8*)main + main->size + 8);
    if (!wmo->prepareLoadedData())
        return false;
    return true;
}
