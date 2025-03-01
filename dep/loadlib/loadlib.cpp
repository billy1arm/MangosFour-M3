//#define _CRT_SECURE_NO_DEPRECATE

#include "loadlib.h"
#include <cstdio>
#include <iostream>

/**
 * @brief Magic number for MVER chunk.
 */
u_map_fcc MverMagic = { {'R','E','V','M'} };

/**
 * @brief Constructor for ChunkedFile.
 */
ChunkedFile::ChunkedFile()
{
    data = 0;
    data_size = 0;
    version = 0;
}

/**
 * @brief Destructor for ChunkedFile.
 */
ChunkedFile::~ChunkedFile()
{
    free();
}

/**
 * @brief Load a file from an MPQ archive.
 * @param mpq The handle to the MPQ archive.
 * @param filename The name of the file to load.
 * @param log Whether to log errors.
 * @return True if the file was loaded successfully, false otherwise.
 */
bool ChunkedFile::loadFile(HANDLE mpq, char* filename, bool log)
{
    free();
    HANDLE file;
    if (!SFileOpenFileEx(mpq, filename, SFILE_OPEN_FROM_MPQ, &file))
    {
        if (log)
        {
            printf("No such file %s\n", filename);
        }
        return false;
    }

    data_size = SFileGetFileSize(file, NULL);
    data = new uint8[data_size];
    SFileReadFile(file, data, data_size, NULL/*bytesRead*/, NULL);
    parseChunks();
    if (prepareLoadedData())
    {
        SFileCloseFile(file);
        return true;
    }

    printf("Error loading %s\n", filename);
    SFileCloseFile(file);
    free();
    return false;
}

/**
 * @brief Load a file from disk.
 * @param filename The name of the file to load.
 * @param log Whether to log errors.
 * @return True if the file was loaded successfully, false otherwise.
 */
bool ChunkedFile::loadFileFromDisk(const char* filename, bool log)
{
    free();

    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        if (log)
        {
            printf("No such file %s\n", filename);
        }
        return false;
    }

    fseek(file, 0, SEEK_END);
    data_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = new uint8[data_size];
    if (!data)
    {
        std::cerr << "Not enough memory for file " << filename << std::endl;
        fclose(file);
        return false;
    }

    if (fread(data, 1, data_size, file) != data_size)
    {
        if (log)
        {
            printf("Can't read file %s\n", filename);
        }
        fclose(file);
        return false;
    }

    fclose(file);

    if (!prepareLoadedData())
    {
        if (log)
        {
            printf("Error loading %s\n", filename);
        }
        return false;
    }

    return true;
}

/**
 * @brief Prepare the loaded data.
 * @return True if the data was prepared successfully, false otherwise.
 */
bool ChunkedFile::prepareLoadedData()
{
    FileChunk* chunk = GetChunk("MVER");
    if (!chunk)
    {
        return false;
    }

    // Check version
    version = chunk->As<file_MVER>();
    if (version->fcc != MverMagic.fcc)
    {
        return false;
    }

    if (version->ver != FILE_FORMAT_VERSION)
    {
        return false;
    }
    return true;
}

/**
 * @brief Free the resources used by ChunkedFile.
 */
void ChunkedFile::free()
{
    for (auto& chunk : chunks)
    {
        delete chunk.second;
    }
    chunks.clear();

    delete[] data;
    data = 0;
    data_size = 0;
    version = 0;
}

/**
 * @brief List of interesting chunk types.
 */
u_map_fcc InterestingChunks[] = {
    { 'R', 'E', 'V', 'M' },
    { 'N', 'I', 'A', 'M' },
    { 'O', '2', 'H', 'M' },
    { 'K', 'N', 'C', 'M' },
    { 'T', 'V', 'C', 'M' },
    { 'Q', 'L', 'C', 'M' }
};

/**
 * @brief Check if a chunk is interesting.
 * @param fcc The chunk type.
 * @return True if the chunk is interesting, false otherwise.
 */
static bool IsInterestingChunk(u_map_fcc const& fcc)
{
    for (u_map_fcc const& f : InterestingChunks)
    {
        if (f.fcc == fcc.fcc)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Parse the chunks in the file.
 */
void ChunkedFile::parseChunks()
{
    uint8* ptr = GetData();
    while (ptr < GetData() + GetDataSize())
    {
        u_map_fcc header = *(u_map_fcc*)ptr;
        uint32 size = 0;
        if (IsInterestingChunk(header))
        {
            size = *(uint32*)(ptr + 4);
            if (size <= data_size)
            {
                std::swap(header.fcc_txt[0], header.fcc_txt[3]);
                std::swap(header.fcc_txt[1], header.fcc_txt[2]);

                FileChunk* chunk = new FileChunk{ ptr, size };
                chunk->parseSubChunks();
                chunks.insert({ std::string(header.fcc_txt, 4), chunk });
            }
        }

        // move to next chunk
        ptr += size + 8;
    }
}

/**
 * @brief Get a chunk by name.
 * @param name The name of the chunk.
 * @return The chunk, or NULL if not found.
 */
FileChunk* ChunkedFile::GetChunk(std::string const& name)
{
    auto range = chunks.equal_range(name);
    if (std::distance(range.first, range.second) == 1)
    {
        return range.first->second;
    }

    return NULL;
}

/**
 * @brief Destructor for FileChunk.
 */
FileChunk::~FileChunk()
{
    for (auto& subchunk : subchunks)
    {
        delete subchunk.second;
    }
    subchunks.clear();
}

/**
 * @brief Parse the subchunks in the chunk.
 */
void FileChunk::parseSubChunks()
{
    uint8* ptr = data + 8; // skip self
    while (ptr < data + size)
    {
        u_map_fcc header = *(u_map_fcc*)ptr;
        uint32 subsize = 0;
        if (IsInterestingChunk(header))
        {
            subsize = *(uint32*)(ptr + 4);
            if (subsize < size)
            {
                std::swap(header.fcc_txt[0], header.fcc_txt[3]);
                std::swap(header.fcc_txt[1], header.fcc_txt[2]);

                FileChunk* chunk = new FileChunk{ ptr, subsize };
                chunk->parseSubChunks();
                subchunks.insert({ std::string(header.fcc_txt, 4), chunk });
            }
        }

        // move to next chunk
        ptr += subsize + 8;
    }
}

/**
 * @brief Get a subchunk by name.
 * @param name The name of the subchunk.
 * @return The subchunk, or NULL if not found.
 */
FileChunk* FileChunk::GetSubChunk(std::string const& name)
{
    auto range = subchunks.equal_range(name);
    if (std::distance(range.first, range.second) == 1)
    {
        return range.first->second;
    }
    return NULL;
}

// list of mpq files for lookup most recent file version
ArchiveSet gOpenArchives;

/**
 * @brief Get the bounds of the open archives.
 * @return The bounds of the open archives.
 */
ArchiveSetBounds GetArchivesBounds()
{
    return ArchiveSetBounds(gOpenArchives.begin(), gOpenArchives.end());
}

/**
 * @brief Open an MPQ archive.
 * @param mpqFileName The name of the MPQ file.
 * @param mpqHandlePtr The handle to the MPQ archive.
 * @return True if the archive was opened successfully, false otherwise.
 */
bool OpenArchive(char const* mpqFileName, HANDLE* mpqHandlePtr /*= NULL*/)
{
    HANDLE mpqHandle;

    if (!SFileOpenArchive(mpqFileName, 0, MPQ_OPEN_READ_ONLY, &mpqHandle))
    {
        return false;
    }

    gOpenArchives.push_back(mpqHandle);

    if (mpqHandlePtr)
    {
        *mpqHandlePtr = mpqHandle;
    }
    return true;
}

/**
 * @brief Open the newest version of a file.
 * @param filename The name of the file.
 * @param fileHandlerPtr The handle to the file.
 * @return True if the file was opened successfully, false otherwise.
 */
bool OpenNewestFile(char const* filename, HANDLE* fileHandlerPtr)
{
    for (ArchiveSet::const_reverse_iterator i = gOpenArchives.rbegin(); i != gOpenArchives.rend(); ++i)
    {
        // always prefer get updated file version
        if (SFileOpenFileEx(*i, filename, SFILE_OPEN_FROM_MPQ, fileHandlerPtr))
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Extract a file from an MPQ archive.
 * @param mpq_name The name of the MPQ file.
 * @param filename The name of the file to extract.
 * @return True if the file was extracted successfully, false otherwise.
 */
bool ExtractFile(char const* mpq_name, std::string const& filename)
{
    for (ArchiveSet::const_reverse_iterator i = gOpenArchives.rbegin(); i != gOpenArchives.rend(); ++i)
    {
        HANDLE fileHandle;
        if (!SFileOpenFileEx(*i, mpq_name, SFILE_OPEN_FROM_MPQ, &fileHandle))
        {
            continue;
        }

        if (SFileGetFileSize(fileHandle, NULL) == 0)              // some files removed in next updates and its reported  size 0
        {
            SFileCloseFile(fileHandle);
            return true;
        }

        SFileCloseFile(fileHandle);

        if (!SFileExtractFile(*i, mpq_name, filename.c_str(), SFILE_OPEN_FROM_MPQ))
        {
            //printf("Can't extract file: %s\n", mpq_name);
            return false;
        }

        return true;
    }

    printf("Extracting file not found: %s\n", filename.c_str());
    return false;
}

/**
 * @brief Extract a file from an MPQ archive with a given handle.
 * @param mpq_name The name of the MPQ file.
 * @param filename The name of the file to extract.
 * @param fileHandle The handle to the file.
 * @return True if the file was extracted successfully, false otherwise.
 */
bool ExtractFile(char const* mpq_name, std::string const& filename, HANDLE fileHandle)
{
    for (ArchiveSet::const_reverse_iterator i = gOpenArchives.rbegin(); i != gOpenArchives.rend(); ++i)
    {
        if (!SFileOpenFileEx(*i, mpq_name, SFILE_OPEN_FROM_MPQ, &fileHandle))
        {
            continue;
        }

        if (SFileGetFileSize(fileHandle, NULL) == 0)              // some files removed in next updates and its reported  size 0
        {
            SFileCloseFile(fileHandle);
            return true;
        }

        SFileCloseFile(fileHandle);

        if (!SFileExtractFile(*i, mpq_name, filename.c_str(), SFILE_OPEN_FROM_MPQ))
        {
            //printf("Can't extract file: %s\n", mpq_name);
            return false;
        }

        return true;
    }

    //printf("Extracting file not found: %s\n", filename.c_str());
    return false;
}

/**
 * @brief Close all open MPQ archives.
 */
void CloseArchives()
{
    for (ArchiveSet::const_iterator i = gOpenArchives.begin(); i != gOpenArchives.end(); ++i)
    {
        SFileCloseArchive(*i);
    }
    gOpenArchives.clear();
}

/**
 * @brief Constructor for FileLoader.
 */
FileLoader::FileLoader()
{
    data = 0;
    data_size = 0;
    version = 0;
}

/**
 * @brief Destructor for FileLoader.
 */
FileLoader::~FileLoader()
{
    free();
}

/**
 * @brief Load a file.
 * @param filename The name of the file to load.
 * @param log Whether to log errors.
 * @return True if the file was loaded successfully, false otherwise.
 */
bool FileLoader::loadFile(char* filename, bool log)
{
    free();

    HANDLE fileHandle = 0;

    if (!OpenNewestFile(filename, &fileHandle))
    {
        if (log)
        {
            printf("No such file %s\n", filename);
        }
        return false;
    }

    data_size = SFileGetFileSize(fileHandle, NULL);

    data = new uint8 [data_size];
    if (!data)
    {
        std::cerr << "Not enough memory for file  " << filename << std::endl;
        SFileCloseFile(fileHandle);
        return false;
    }

    if (!SFileReadFile(fileHandle, data, data_size, NULL, NULL))
    {
        if (log)
        {
            printf("Can't read file %s\n", filename);
        }
        SFileCloseFile(fileHandle);
        return false;
    }

    // ToDo: Fix WDT errors...
    if (!prepareLoadedData())
    {
        //printf("Error loading %s\n\n", filename);
        SFileCloseFile(fileHandle);
        return false;
    }

    SFileCloseFile(fileHandle);

    return true;
}

/**
 * @brief Load a file from disk.
 * @param filename The name of the file to load.
 * @param log Whether to log errors.
 * @return True if the file was loaded successfully, false otherwise.
 */
bool FileLoader::loadFileFromDisk(const char* filename, bool log)
{
    free();

    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        if (log)
        {
            printf("No such file %s\n", filename);
        }
        return false;
    }

    fseek(file, 0, SEEK_END);
    data_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    data = new uint8[data_size];
    if (!data)
    {
        std::cerr << "Not enough memory for file " << filename << std::endl;
        fclose(file);
        return false;
    }

    if (fread(data, 1, data_size, file) != data_size)
    {
        if (log)
        {
            printf("Can't read file %s\n", filename);
        }
        fclose(file);
        return false;
    }

    fclose(file);

    if (!prepareLoadedData())
    {
        //printf("Error loading %s\n\n", filename);
        //free();
        return false;
    }

    return true;
}

/**
 * @brief Prepare the loaded data.
 * @return True if the data was prepared successfully, false otherwise.
 */
bool FileLoader::prepareLoadedData()
{
    // Check version
    version = (file_MVER*) data;
    if (version->fcc != 'MVER')
    {
        return false;
    }
    if (version->ver != FILE_FORMAT_VERSION)
    {
        return false;
    }
    return true;
}

/**
 * @brief Free the resources used by FileLoader.
 */
void FileLoader::free()
{
    if (data)
    {
        delete[] data;
    }
    data = 0;
    data_size = 0;
    version = 0;
}

