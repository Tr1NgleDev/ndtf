#ifndef _NDTF_H_
#define _NDTF_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define NDTF_SIGNATURE "NDTF"
#define NDTF_CREATE_VERSION(major, minor) ( ((major & 0xFF) << 8) | (minor & 0xFF) )
#define NDTF_VERSION_MAJOR 1
#define NDTF_VERSION_MINOR 0
#define NDTF_VERSION NDTF_CREATE_VERSION(NDTF_VERSION_MAJOR, NDTF_VERSION_MINOR)
#define NDTF_EXTRACT_VERSION_MAJOR(version) ( version & 0xFF00 )
#define NDTF_EXTRACT_VERSION_MINOR(version) ( version & 0x00FF )

typedef enum NDTF_Dimensions
{
	NDTF_DIMENSIONS_TWO = 2,
	NDTF_DIMENSIONS_THREE = 3,
	NDTF_DIMENSIONS_FOUR = 4,
	NDTF_DIMENSIONS_FIVE = 5,
	NDTF_DIMENSIONS_MIN = NDTF_DIMENSIONS_TWO,
	NDTF_DIMENSIONS_MAX = NDTF_DIMENSIONS_FIVE,
} NDTF_Dimensions;

typedef enum NDTF_TexelFormat
{
	NDTF_TEXELFORMAT_NONE = 0,			// NONE
	NDTF_TEXELFORMAT_RGBA8888 = 1,		// RGBA UNSIGNED_BYTE		(0-255)
	NDTF_TEXELFORMAT_RGB888,			// RGB UNSIGNED_BYTE		(0-255)
	NDTF_TEXELFORMAT_R8,				// R UNSIGNED_BYTE			(0-255)
	NDTF_TEXELFORMAT_RGBA16161616,		// RGBA UNSIGNED_SHORT		(0-65535)
	NDTF_TEXELFORMAT_RGB161616,			// RGB UNSIGNED_SHORT		(0-65535)
	NDTF_TEXELFORMAT_R16,				// R UNSIGNED_SHORT			(0-65535)
	NDTF_TEXELFORMAT_RGBA32323232F,		// RGBA FLOAT				(-INF - INF)
	NDTF_TEXELFORMAT_RGB323232F,		// RGB FLOAT				(-INF - INF)
	NDTF_TEXELFORMAT_R32F,				// R FLOAT					(-INF - INF)
	NDTF_TEXELFORMAT_RGBA32323232,		// RGBA UNSIGNED_INTEGER	(0-4294967295)
	NDTF_TEXELFORMAT_RGB323232,			// RGB UNSIGNED_INTEGER		(0-4294967295)
	NDTF_TEXELFORMAT_R32,				// R UNSIGNED_INTEGER		(0-4294967295)

	NDTF_TEXELFORMAT_XYZW32323232F = NDTF_TEXELFORMAT_RGBA32323232F,		// (alias) XYZW FLOAT				(-INF - INF)
	NDTF_TEXELFORMAT_XYZ323232F = NDTF_TEXELFORMAT_RGB323232F,				// (alias) XYZ FLOAT				(-INF - INF)
} NDTF_TexelFormat;

typedef enum NDTF_Channels
{
	NDTF_CHANNELS_NONE = 0,
	NDTF_CHANNELS_R = 1,
	NDTF_CHANNELS_RGB = 3,
	NDTF_CHANNELS_RGBA = 4,
	NDTF_CHANNELS_XYZ = NDTF_CHANNELS_RGB,
	NDTF_CHANNELS_XYZW = NDTF_CHANNELS_RGBA,
} NDTF_Channels;

typedef struct NDTF_Flags
{
	uint32_t zlib_compression : 1;
	uint32_t __unused__ : 31;
} NDTF_Flags;

typedef struct NDTF_Header
{
	char signature[4];	// NDTF_SIGNATURE (NDTF)
	uint16_t version;	// NDTF_VERSION
	uint8_t dimensions;	// enum NDTF_Dimensions
	uint8_t texelFormat;// enum NDTF_TexelFormat
	NDTF_Flags flags;
	union
	{
		struct { uint16_t size[NDTF_DIMENSIONS_MAX]; };
		struct { uint16_t width, height, depth, ind, ind2; };
	}; // size (width, height, depth, ind, ind2)
	uint8_t __padding__[10];
} NDTF_Header;

typedef struct NDTF_File
{
	NDTF_Header header;
	union
	{
		uint8_t* data;
		uint8_t* data8b;
		uint16_t* data16b;
		uint32_t* data32b;
		float* dataf;
	}; // data
} NDTF_File;

typedef struct NDTF_Coord
{
	union
	{
		struct { uint16_t coord[NDTF_DIMENSIONS_MAX]; };
		struct { uint16_t x, y, z, w, v; };
	};
} NDTF_Coord;

#ifdef __cplusplus
extern "C" {
#endif
	NDTF_Channels ndtf_getChannelCount(NDTF_TexelFormat texelFormat);
	size_t ndtf_getChannelSize(NDTF_TexelFormat texelFormat);
	size_t ndtf_getTexelSize(NDTF_TexelFormat texelFormat);
	bool ndtf_getChannelIsFloat(NDTF_TexelFormat texelFormat);
	NDTF_File ndtf_file_loadFromData(uint8_t* data, size_t size, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	NDTF_File ndtf_file_loadFromFile(FILE* file, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	NDTF_File ndtf_file_load(const char* filename, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	void* ndtf_loadFromData(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	void* ndtf_loadFromFile(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	void* ndtf_load(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint8_t* ndtf_loadFromData_u8(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint8_t* ndtf_loadFromFile_u8(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint8_t* ndtf_load_u8(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint16_t* ndtf_loadFromData_u16(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint16_t* ndtf_loadFromFile_u16(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint16_t* ndtf_load_u16(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint32_t* ndtf_loadFromData_u32(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint32_t* ndtf_loadFromFile_u32(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	uint32_t* ndtf_load_u32(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	float* ndtf_loadFromData_f(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	float* ndtf_loadFromFile_f(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	float* ndtf_load_f(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat);
	bool ndtf_file_isValid(NDTF_File* file);
	void ndtf_file_reformat(NDTF_File* file, NDTF_TexelFormat desiredFormat);
	NDTF_File ndtf_file_create(NDTF_Dimensions dimensions, NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height, uint16_t depth, uint16_t ind, uint16_t ind2);
	NDTF_File ndtf_file_create_2D(NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height);
	NDTF_File ndtf_file_create_3D(NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height, uint16_t depth);
	NDTF_File ndtf_file_create_4D(NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height, uint16_t depth, uint16_t ind);
	NDTF_File ndtf_file_create_5D(NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height, uint16_t depth, uint16_t ind, uint16_t ind2);
	uint64_t ndtf_file_getTexelIndex(NDTF_File* file, NDTF_Coord* coordPtr);
	bool ndtf_file_setTexel(NDTF_File* file, NDTF_Coord* coordPtr, void* colorPtr);
	bool ndtf_file_setTexel_2D(NDTF_File* file, uint16_t x, uint16_t y, void* colorPtr);
	bool ndtf_file_setTexel_3D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, void* colorPtr);
	bool ndtf_file_setTexel_4D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, uint16_t w, void* colorPtr);
	bool ndtf_file_setTexel_5D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, uint16_t w, uint16_t v, void* colorPtr);
	void* ndtf_file_getTexel(NDTF_File* file, NDTF_Coord* coordPtr);
	void* ndtf_file_getTexel_2D(NDTF_File* file, uint16_t x, uint16_t y);
	void* ndtf_file_getTexel_3D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z);
	void* ndtf_file_getTexel_4D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, uint16_t w);
	void* ndtf_file_getTexel_5D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, uint16_t w, uint16_t v);
	void* ndtf_file_saveToData(NDTF_File* file, uint64_t* size);
	bool ndtf_file_saveToFile(NDTF_File* file, FILE* handle);
	bool ndtf_file_save(NDTF_File* file, const char* filename);

	// GL helpers
#ifdef NDTF_GL_HELPER_FUNCTIONS

	GLenum ndtf_glSizedTexelFormat(NDTF_TexelFormat texelFormat);
	GLenum ndtf_glTexelFormat(NDTF_TexelFormat texelFormat);
	GLenum ndtf_glTexelType(NDTF_TexelFormat texelFormat);

#endif // NDTF_GL_HELPER_FUNCTIONS

#ifdef __cplusplus
}
#endif

#endif // !_NDTF_H_
