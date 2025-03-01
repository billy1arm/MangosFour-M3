#include "mpq.h"
#include "StormLib.h"

/**
 * @brief Constructor for MPQFile.
 * @param file The handle to the MPQ file.
 * @param filename The name of the file.
 */
MPQFile::MPQFile(HANDLE file, const char* filename):
    eof(false),
    buffer(0),
    pointer(0),
    size(0)
{
    DWORD hi = 0;
    size = SFileGetFileSize(file, &hi);

    if (hi)
    {
        fprintf(stderr, "Can't open %s, size[hi] = %u!\n", filename, (uint32)hi);
        SFileCloseFile(file);
        eof = true;
        return;
    }

    if (size <= 1)
    {
        fprintf(stderr, "Can't open %s, size = %zu!\n", filename, size);
        SFileCloseFile(file);
        eof = true;
        return;
    }

    DWORD read = 0;
    buffer = new char[size];
    if (!SFileReadFile(file, buffer, size, &read, NULL) || size != read)
    {
        fprintf(stderr, "Can't read %s, size=%zu read=%u!\n", filename, size, read);
        SFileCloseFile(file);
        eof = true;
        return;
    }

    SFileCloseFile(file);
}

/**
 * @brief Read data from the MPQ file.
 * @param dest The destination buffer.
 * @param bytes The number of bytes to read.
 * @return The number of bytes read.
 */
size_t MPQFile::read(void* dest, size_t bytes)
{
    if (eof) return 0;

    size_t rpos = pointer + bytes;
    if (rpos > size)
    {
        bytes = size - pointer;
        eof = true;
    }

    memcpy(dest, &(buffer[pointer]), bytes);

    pointer = rpos;

    return bytes;
}

/**
 * @brief Set the file pointer to a specific offset.
 * @param offset The offset to set the pointer to.
 */
void MPQFile::seek(int offset)
{
    pointer = offset;
    eof = (pointer >= size);
}

/**
 * @brief Move the file pointer by a relative offset.
 * @param offset The relative offset to move the pointer by.
 */
void MPQFile::seekRelative(int offset)
{
    pointer += offset;
    eof = (pointer >= size);
}

/**
 * @brief Close the MPQ file and free resources.
 */
void MPQFile::close()
{
    if (buffer) delete[] buffer;
    buffer = 0;
    eof = true;
}

