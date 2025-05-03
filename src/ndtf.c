#include <ndtf/ndtf.h>
#include <string.h>
#include <zlib.h>

#define _CRT_SECURE_NO_DEPRECATE

#ifndef max
	#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
	#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

NDTF_Channels ndtf_getChannelCount(NDTF_TexelFormat texelFormat)
{
	switch (texelFormat)
	{
	case NDTF_TEXELFORMAT_RGBA8888:
	case NDTF_TEXELFORMAT_RGBA16161616:
	case NDTF_TEXELFORMAT_RGBA32323232F:
	case NDTF_TEXELFORMAT_RGBA32323232:
		return NDTF_CHANNELS_RGBA;
	case NDTF_TEXELFORMAT_RGB888:
	case NDTF_TEXELFORMAT_RGB161616:
	case NDTF_TEXELFORMAT_RGB323232F:
	case NDTF_TEXELFORMAT_RGB323232:
		return NDTF_CHANNELS_RGB;
	case NDTF_TEXELFORMAT_R8:
	case NDTF_TEXELFORMAT_R16:
	case NDTF_TEXELFORMAT_R32F:
	case NDTF_TEXELFORMAT_R32:
		return NDTF_CHANNELS_R;
	default:
		return NDTF_CHANNELS_NONE;
	}
}
size_t ndtf_getChannelSize(NDTF_TexelFormat texelFormat)
{
	switch (texelFormat)
	{
	case NDTF_TEXELFORMAT_RGBA8888:
	case NDTF_TEXELFORMAT_RGB888:
	case NDTF_TEXELFORMAT_R8:
		return 1;
	case NDTF_TEXELFORMAT_RGBA16161616:
	case NDTF_TEXELFORMAT_RGB161616:
	case NDTF_TEXELFORMAT_R16:
		return 2;
	case NDTF_TEXELFORMAT_RGBA32323232:
	case NDTF_TEXELFORMAT_RGBA32323232F:
	case NDTF_TEXELFORMAT_RGB323232:
	case NDTF_TEXELFORMAT_RGB323232F:
	case NDTF_TEXELFORMAT_R32:
	case NDTF_TEXELFORMAT_R32F:
		return 4;
	default:
		return 0;
	}
}
size_t ndtf_getTexelSize(NDTF_TexelFormat texelFormat)
{
	switch (texelFormat)
	{
	case NDTF_TEXELFORMAT_RGBA8888:
		return 4;
	case NDTF_TEXELFORMAT_RGB888:
		return 3;
	case NDTF_TEXELFORMAT_R8:
		return 1;
	case NDTF_TEXELFORMAT_RGBA16161616:
		return 8;
	case NDTF_TEXELFORMAT_RGB161616:
		return 6;
	case NDTF_TEXELFORMAT_R16:
		return 2;
	case NDTF_TEXELFORMAT_RGBA32323232:
	case NDTF_TEXELFORMAT_RGBA32323232F:
		return 16;
	case NDTF_TEXELFORMAT_RGB323232:
	case NDTF_TEXELFORMAT_RGB323232F:
		return 12;
	case NDTF_TEXELFORMAT_R32:
	case NDTF_TEXELFORMAT_R32F:
		return 4;
	default:
		return 0;
	}
}
bool ndtf_getChannelIsFloat(NDTF_TexelFormat texelFormat)
{
	switch (texelFormat)
	{
	case NDTF_TEXELFORMAT_RGBA32323232F:
	case NDTF_TEXELFORMAT_RGB323232F:
	case NDTF_TEXELFORMAT_R32F:
		return true;
	default:
		return false;
	}
}

NDTF_File ndtf_file_loadFromData(uint8_t* data, size_t size, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_File result;
	memset(&result, 0, sizeof(NDTF_File));

	if (size < sizeof(NDTF_Header))
		return result;

	memcpy(&result.header, data, sizeof(NDTF_Header));

	if (memcmp(result.header.signature, NDTF_SIGNATURE, 4) != 0)
	{
		memset(&result, 0, sizeof(NDTF_File));
		return result;
	}

	if (result.header.version > NDTF_VERSION)
	{
		memset(&result, 0, sizeof(NDTF_File));
		return result;
	}

	if (result.header.dimensions < NDTF_DIMENSIONS_MIN || result.header.dimensions > NDTF_DIMENSIONS_MAX)
	{
		memset(&result, 0, sizeof(NDTF_File));
		return result;
	}

	size_t dataSize = ndtf_file_getDataSize(&result);

	if (ndtf_file_getZLibCompression(&result))
	{
		size_t actualDataSize = 0;
		result.data = ndtf_zLibDecompressData(data + sizeof(NDTF_Header), size - sizeof(NDTF_Header), &actualDataSize);
		if (!result.data)
		{
			memset(&result, 0, sizeof(NDTF_File));
			return result;
		}
		if (actualDataSize != dataSize)
		{
			free(result.data);
			memset(&result, 0, sizeof(NDTF_File));
			return result;
		}
	}
	else
	{
		if (size != sizeof(NDTF_Header) + dataSize)
		{
			memset(&result, 0, sizeof(NDTF_File));
			return result;
		}

		result.data = (uint8_t*)malloc(dataSize);
		if (!result.data)
		{
			memset(&result, 0, sizeof(NDTF_File));
			return result;
		}

		memcpy(result.data, data + sizeof(NDTF_Header), dataSize);
	}

	if (format) *format = (NDTF_TexelFormat)result.header.texelFormat;
	ndtf_file_reformat(&result, desiredFormat);

	return result;
}
NDTF_File ndtf_file_loadFromFile(FILE* file, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_File result;
	memset(&result, 0, sizeof(NDTF_File));

	if (file != NULL)
	{
		if (fseek(file, 0, SEEK_END) != 0)
			return result;

		int64_t size = ftell(file);
		if (size < 0)
			return result;

		if (fseek(file, 0, SEEK_SET) != 0)
			return result;

		uint8_t* data = (uint8_t*)malloc((size_t)size);
		if (!data)
			return result;

		size_t bytesRead = fread(data, 1, size, file);

		if (bytesRead < (size_t)size)
		{
			if (ferror(file))
			{
				free(data);
				return result;
			}
		}

		result = ndtf_file_loadFromData(data, (size_t)size, format, desiredFormat);

		free(data);
	}

	return result;
}
NDTF_File ndtf_file_load(const char* filename, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_File result;

	FILE* file = fopen(filename, "rb");

	if (file != NULL)
	{
		result = ndtf_file_loadFromFile(file, format, desiredFormat);
		fclose(file);
	}
	else
		memset(&result, 0, sizeof(NDTF_File));

	return result;
}

void* ndtf_loadFromData(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_File f = ndtf_file_loadFromData(data, size, format, desiredFormat);
	if (!ndtf_file_isValid(&f)) return NULL;

	if (width)
		*width = max(f.header.width, 1);
	if (height)
		*height = max(f.header.height, 1);
	if (depth)
		*depth = max(f.header.depth, 1);
	if (ind)
		*ind = max(f.header.ind, 1);
	if (ind2)
		*ind2 = max(f.header.ind2, 1);

	return f.data8b;
}
void* ndtf_loadFromFile(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_File f = ndtf_file_loadFromFile(file, format, desiredFormat);
	if (!ndtf_file_isValid(&f)) return NULL;

	if (width)
		*width = max(f.header.width, 1);
	if (height)
		*height = max(f.header.height, 1);
	if (depth)
		*depth = max(f.header.depth, 1);
	if (ind)
		*ind = max(f.header.ind, 1);
	if (ind2)
		*ind2 = max(f.header.ind2, 1);

	return f.data8b;
}
void* ndtf_load(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_File f = ndtf_file_load(filename, format, desiredFormat);
	if (!ndtf_file_isValid(&f)) return NULL;

	if (width)
		*width = max(f.header.width, 1);
	if (height)
		*height = max(f.header.height, 1);
	if (depth)
		*depth = max(f.header.depth, 1);
	if (ind)
		*ind = max(f.header.ind, 1);
	if (ind2)
		*ind2 = max(f.header.ind2, 1);

	return f.data8b;
}
uint8_t* ndtf_loadFromData_u8(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_loadFromData(data, size, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 1 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint8_t*)result;
}
uint8_t* ndtf_loadFromFile_u8(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_loadFromFile(file, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 1 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint8_t*)result;
}
uint8_t* ndtf_load_u8(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_load(filename, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 1 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint8_t*)result;
}
uint16_t* ndtf_loadFromData_u16(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_loadFromData(data, size, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 2 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint16_t*)result;
}
uint16_t* ndtf_loadFromFile_u16(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_loadFromFile(file, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 2 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint16_t*)result;
}
uint16_t* ndtf_load_u16(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_load(filename, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 2 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint16_t*)result;
}
uint32_t* ndtf_loadFromData_u32(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_loadFromData(data, size, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 4 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint32_t*)result;
}
uint32_t* ndtf_loadFromFile_u32(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_loadFromFile(file, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 4 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint32_t*)result;
}
uint32_t* ndtf_load_u32(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_load(filename, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 4 && !ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (uint32_t*)result;
}
float* ndtf_loadFromData_f(uint8_t* data, size_t size, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_loadFromData(data, size, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 4 && ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (float*)result;
}
float* ndtf_loadFromFile_f(FILE* file, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_loadFromFile(file, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 4 && ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (float*)result;
}
float* ndtf_load_f(const char* filename, uint16_t* width, uint16_t* height, uint16_t* depth, uint16_t* ind, uint16_t* ind2, NDTF_TexelFormat* format, NDTF_TexelFormat desiredFormat)
{
	NDTF_TexelFormat f;
	void* result = ndtf_load(filename, width, height, depth, ind, ind2, &f, desiredFormat);

	if (result)
	{
		if (ndtf_getChannelSize(f) == 4 && ndtf_getChannelIsFloat(f))
		{
			free(result);
			return NULL;
		}

		if (format)
			*format = f;
	}

	return (float*)result;
}

bool ndtf_file_isValid(NDTF_File* file)
{
	return file->data && file->header.width && file->header.height;
}

void ndtf_file_reformat(NDTF_File* file, NDTF_TexelFormat desiredFormat)
{
	size_t totalTexels = 1;
	for (int i = 0; i < file->header.dimensions; i++)
		totalTexels *= file->header.size[i];

	size_t bpp = ndtf_getTexelSize((NDTF_TexelFormat)file->header.texelFormat);
	size_t tDataSize = totalTexels * bpp;

	if (desiredFormat != NDTF_TEXELFORMAT_NONE && (NDTF_TexelFormat)file->header.texelFormat != desiredFormat)
	{
		union
		{
			uint8_t* data;
			uint8_t* data8b;
			uint16_t* data16b;
			uint32_t* data32b;
			float* dataf;
		} oldData;
		oldData.data = file->data;

		int8_t oldFormat = file->header.texelFormat;
		file->header.texelFormat = desiredFormat;

		size_t nBPP = ndtf_getTexelSize((NDTF_TexelFormat)file->header.texelFormat);
		size_t nTDataSize = totalTexels * nBPP;
		file->data = (uint8_t*)malloc(nTDataSize);

		NDTF_Channels oldChannels = ndtf_getChannelCount((NDTF_TexelFormat)oldFormat);
		NDTF_Channels newChannels = ndtf_getChannelCount((NDTF_TexelFormat)desiredFormat);
		size_t oldChannelSize = ndtf_getChannelSize((NDTF_TexelFormat)oldFormat);
		size_t newChannelSize = ndtf_getChannelSize((NDTF_TexelFormat)desiredFormat);
		bool oldCIsFloat = ndtf_getChannelIsFloat((NDTF_TexelFormat)oldFormat);
		bool newCIsFloat = ndtf_getChannelIsFloat((NDTF_TexelFormat)desiredFormat);

		for (size_t i = 0; i < tDataSize; i += bpp)
		{
			size_t t = i / bpp;

			for (int j = 0; j < newChannels; j++)
			{
				switch (oldChannelSize)
				{
				case 1:
				{
					switch (newChannelSize)
					{
					case 1:
					{
						if (j < oldChannels)
							file->data8b[t * newChannels + j] = oldData.data8b[t * oldChannels + j];
						else
							file->data8b[t * newChannels + j] = UINT8_MAX;
					} break;
					case 2:
					{
						if (j < oldChannels)
							file->data16b[t * newChannels + j] = (size_t)oldData.data8b[t * oldChannels + j] * UINT16_MAX / UINT8_MAX;
						else
							file->data16b[t * newChannels + j] = UINT16_MAX;
					} break;
					case 4:
					{
						if (newCIsFloat)
						{
							if (j < oldChannels)
								file->dataf[t * newChannels + j] = (float)oldData.data8b[t * oldChannels + j] / UINT8_MAX;
							else
								file->dataf[t * newChannels + j] = 1.0f;
						}
						else
						{
							if (j < oldChannels)
								file->data32b[t * newChannels + j] = (size_t)oldData.data8b[t * oldChannels + j] * UINT32_MAX / UINT8_MAX;
							else
								file->data32b[t * newChannels + j] = UINT32_MAX;
						}
					} break;
					}
				} break;
				case 2:
				{
					switch (newChannelSize)
					{
					case 1:
					{
						if (j < oldChannels)
							file->data8b[t * newChannels + j] = oldData.data8b[t * oldChannels + j] * UINT8_MAX / UINT16_MAX;
						else
							file->data8b[t * newChannels + j] = UINT8_MAX;
					} break;
					case 2:
					{
						if (j < oldChannels)
							file->data16b[t * newChannels + j] = oldData.data16b[t * oldChannels + j];
						else
							file->data16b[t * newChannels + j] = UINT16_MAX;
					} break;
					case 4:
					{
						if (newCIsFloat)
						{
							if (j < oldChannels)
								file->dataf[t * newChannels + j] = (float)oldData.data16b[t * oldChannels + j] / UINT16_MAX;
							else
								file->dataf[t * newChannels + j] = 1.0f;
						}
						else
						{
							if (j < oldChannels)
								file->data32b[t * newChannels + j] = (size_t)oldData.data16b[t * oldChannels + j] * UINT32_MAX / UINT16_MAX;
							else
								file->data32b[t * newChannels + j] = UINT32_MAX;
						}
					} break;
					}
				} break;
				case 4:
				{
					switch (newChannelSize)
					{
					case 1:
					{
						if (j < oldChannels)
							file->data8b[t * newChannels + j] = (uint8_t)(oldCIsFloat ? oldData.dataf[t * oldChannels + j] * UINT8_MAX : oldData.data32b[t * oldChannels + j] / 0x1000000u);
						else
							file->data8b[t * newChannels + j] = UINT8_MAX;
					} break;
					case 2:
					{
						if (j < oldChannels)
							file->data16b[t * newChannels + j] = (uint16_t)(oldCIsFloat ? oldData.dataf[t * oldChannels + j] * UINT16_MAX : oldData.data32b[t * oldChannels + j] / 0x10000u);
						else
							file->data16b[t * newChannels + j] = UINT16_MAX;
					} break;
					case 4:
					{
						if (newCIsFloat)
						{
							if (j < oldChannels)
								file->dataf[t * newChannels + j] = (oldCIsFloat ? oldData.dataf[t * oldChannels + j] : (float)oldData.data32b[t * oldChannels + j] / UINT32_MAX);
							else
								file->dataf[t * newChannels + j] = 1.0f;
						}
						else
						{
							if (j < oldChannels)
								file->data32b[t * newChannels + j] = (uint32_t)(oldCIsFloat ? oldData.dataf[t * oldChannels + j] * UINT32_MAX : oldData.data32b[t * oldChannels + j]);
							else
								file->data32b[t * newChannels + j] = UINT32_MAX;
						}
					} break;
					}
				} break;
				}
			}
		}

		free(oldData.data); // remove original file data
	}
}

NDTF_File ndtf_file_create(NDTF_Dimensions dimensions, NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height, uint16_t depth, uint16_t ind, uint16_t ind2)
{
	NDTF_File result;
	memset(&result, 0, sizeof(NDTF_File));

	memcpy(result.header.signature, NDTF_SIGNATURE, 4);
	result.header.version = NDTF_VERSION;
	result.header.dimensions = dimensions;
	result.header.texelFormat = texelFormat;
	result.header.width = max(width, 1);
	result.header.height = max(height, 1);
	result.header.depth = max(depth, 1);
	result.header.ind = max(ind, 1);
	result.header.ind2 = max(ind2, 1);

	size_t totalTexels = 1;
	for (int i = 0; i < dimensions; i++)
		totalTexels *= result.header.size[i];

	size_t bpp = ndtf_getTexelSize(texelFormat);
	size_t tDataSize = totalTexels * bpp;

	result.data = (uint8_t*)malloc(tDataSize);

	return result;
}
NDTF_File ndtf_file_create_2D(NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height)
{
	return ndtf_file_create(NDTF_DIMENSIONS_TWO, texelFormat, width, height, 1, 1, 1);
}
NDTF_File ndtf_file_create_3D(NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height, uint16_t depth)
{
	return ndtf_file_create(NDTF_DIMENSIONS_THREE, texelFormat, width, height, depth, 1, 1);
}
NDTF_File ndtf_file_create_4D(NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height, uint16_t depth, uint16_t ind)
{
	return ndtf_file_create(NDTF_DIMENSIONS_FOUR, texelFormat, width, height, depth, ind, 1);
}
NDTF_File ndtf_file_create_5D(NDTF_TexelFormat texelFormat, uint16_t width, uint16_t height, uint16_t depth, uint16_t ind, uint16_t ind2)
{
	return ndtf_file_create(NDTF_DIMENSIONS_FIVE, texelFormat, width, height, depth, ind, ind2);
}
size_t ndtf_file_getTexelIndex(NDTF_File* file, NDTF_Coord* coordPtr)
{
	NDTF_Channels channels = ndtf_getChannelCount((NDTF_TexelFormat)file->header.texelFormat);

	size_t ind = 0;
	for (int i = 0; i < file->header.dimensions; i++)
	{
		size_t sp = 1;
		for (int j = (uint8_t)NDTF_DIMENSIONS_TWO - 1; j <= i; j++)
		{
			if (j >= 1)
				sp *= file->header.size[j - 1];
		}
		ind += coordPtr->coord[i] * max(sp, 1);
	}

	ind *= (size_t)channels;

	return ind;
}
bool ndtf_file_setTexel(NDTF_File* file, NDTF_Coord* coordPtr, void* colorPtr)
{
	NDTF_Channels channels = ndtf_getChannelCount((NDTF_TexelFormat)file->header.texelFormat);
	size_t channelSize = ndtf_getChannelSize((NDTF_TexelFormat)file->header.texelFormat);
	bool cIsFloat = ndtf_getChannelIsFloat((NDTF_TexelFormat)file->header.texelFormat);

	size_t ind = ndtf_file_getTexelIndex(file, coordPtr);

	size_t dataSize = ndtf_file_getDataSize(file);

	if (ind >= dataSize) return false;

	switch (channelSize)
	{
	case 1:
	{
		for (int i = 0; i < (int)channels; i++)
			file->data8b[ind + i] = ((uint8_t*)colorPtr)[i];
	} break;
	case 2:
	{
		for (int i = 0; i < (int)channels; i++)
			file->data16b[ind + i] = ((uint16_t*)colorPtr)[i];
	} break;
	case 4:
	{
		if (cIsFloat)
		{
			for (int i = 0; i < (int)channels; i++)
				file->dataf[ind + i] = ((float*)colorPtr)[i];
		}
		else
		{
			for (int i = 0; i < (int)channels; i++)
				file->data32b[ind + i] = ((uint32_t*)colorPtr)[i];
		}
	} break;
	}

	return true;
}
bool ndtf_file_setTexel_2D(NDTF_File* file, uint16_t x, uint16_t y, void* colorPtr)
{
	NDTF_Coord coord;
	coord.x = x;
	coord.y = y;
	coord.z = 0;
	coord.w = 0;
	coord.v = 0;
	return ndtf_file_setTexel(file, &coord, colorPtr);
}
bool ndtf_file_setTexel_3D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, void* colorPtr)
{
	NDTF_Coord coord;
	coord.x = x;
	coord.y = y;
	coord.z = z;
	coord.w = 0;
	coord.v = 0;
	return ndtf_file_setTexel(file, &coord, colorPtr);
}
bool ndtf_file_setTexel_4D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, uint16_t w, void* colorPtr)
{
	NDTF_Coord coord;
	coord.x = x;
	coord.y = y;
	coord.z = z;
	coord.w = w;
	coord.v = 0;
	return ndtf_file_setTexel(file, &coord, colorPtr);
}
bool ndtf_file_setTexel_5D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, uint16_t w, uint16_t v, void* colorPtr)
{
	NDTF_Coord coord;
	coord.x = x;
	coord.y = y;
	coord.z = z;
	coord.w = w;
	coord.v = v;
	return ndtf_file_setTexel(file, &coord, colorPtr);
}

void* ndtf_file_getTexel(NDTF_File* file, NDTF_Coord* coordPtr)
{
	size_t ind = ndtf_file_getTexelIndex(file, coordPtr);

	size_t totalTexels = 1;
	for (int i = 0; i < file->header.dimensions; i++)
		totalTexels *= file->header.size[i];
	size_t bpp = ndtf_getTexelSize((NDTF_TexelFormat)file->header.texelFormat);
	size_t tDataSize = totalTexels * bpp;

	if (ind >= tDataSize) return NULL;

	return (void*)&file->data[ind];
}
void* ndtf_file_getTexel_2D(NDTF_File* file, uint16_t x, uint16_t y)
{
	NDTF_Coord coord;
	coord.x = x;
	coord.y = y;
	coord.z = 0;
	coord.w = 0;
	coord.v = 0;
	return ndtf_file_getTexel(file, &coord);
}
void* ndtf_file_getTexel_3D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z)
{
	NDTF_Coord coord;
	coord.x = x;
	coord.y = y;
	coord.z = z;
	coord.w = 0;
	coord.v = 0;
	return ndtf_file_getTexel(file, &coord);
}
void* ndtf_file_getTexel_4D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, uint16_t w)
{
	NDTF_Coord coord;
	coord.x = x;
	coord.y = y;
	coord.z = z;
	coord.w = w;
	coord.v = 0;
	return ndtf_file_getTexel(file, &coord);
}
void* ndtf_file_getTexel_5D(NDTF_File* file, uint16_t x, uint16_t y, uint16_t z, uint16_t w, uint16_t v)
{
	NDTF_Coord coord;
	coord.x = x;
	coord.y = y;
	coord.z = z;
	coord.w = w;
	coord.v = v;
	return ndtf_file_getTexel(file, &coord);
}
void* ndtf_file_saveToData(NDTF_File* file, size_t* size)
{
	if (!ndtf_file_isValid(file)) return NULL;

	size_t fileSize = sizeof(NDTF_Header);

	size_t dataSize = ndtf_file_getDataSize(file);

	fileSize += dataSize;

	uint8_t* data = (uint8_t*)malloc(fileSize);

	if (!data) return NULL;

	memcpy(data, &file->header, sizeof(NDTF_Header));

	void* fileData = file->data;
	if (ndtf_file_getZLibCompression(file))
		fileData = ndtf_zLibCompressData(fileData, dataSize, &dataSize);
	if (!fileData) return NULL;

	memcpy(data + sizeof(NDTF_Header), fileData, dataSize);

	if (ndtf_file_getZLibCompression(file))
		free(fileData);

	if (size)
		*size = fileSize;

	return data;
}
bool ndtf_file_saveToFile(NDTF_File* file, FILE* handle)
{
	if (!ndtf_file_isValid(file)) return false;

	if (!handle) return false;

	size_t bytesWritten;

	bytesWritten = fwrite(&file->header, sizeof(uint8_t), sizeof(NDTF_Header), handle);
	if (bytesWritten < sizeof(NDTF_Header)) return false;

	size_t dataSize = ndtf_file_getDataSize(file);

	void* fileData = file->data;
	if (ndtf_file_getZLibCompression(file))
		fileData = ndtf_zLibCompressData(fileData, dataSize, &dataSize);
	if (!fileData) return false;

	bytesWritten = fwrite(fileData, sizeof(uint8_t), dataSize, handle);
	
	if (ndtf_file_getZLibCompression(file))
		free(fileData);

	if (bytesWritten < dataSize) return false;

	return true;
}
bool ndtf_file_save(NDTF_File* file, const char* filename)
{
	if (!ndtf_file_isValid(file)) return false;

	FILE* handle = NULL;
	fopen_s(&handle, filename, "wb");

	if (handle != NULL)
	{
		if (!ndtf_file_saveToFile(file, handle))
		{
			fclose(handle);
			return false;
		}
		fclose(handle);
		return true;
	}

	return false;
}


bool ndtf_file_getZLibCompression(NDTF_File* file)
{
	return file->header.flags.zlib_compression;
}

void ndtf_file_setZLibCompression(NDTF_File* file, bool zlib_compression)
{
	file->header.flags.zlib_compression = zlib_compression;
}

void* ndtf_zLibCompressData(const void* data, size_t size, size_t* newSize)
{
	uint64_t compSize = compressBound(size);

	uint8_t* compData = (uint8_t*)malloc(compSize + sizeof(uint64_t));
	if (!compData) return NULL;

	int result = compress(compData + sizeof(uint64_t), &compSize, data, size);
	if (result != Z_OK)
	{
		free(compData);
		return NULL;
	}

	memcpy((void*)compData, (void*)&size, sizeof(uint64_t)); // store the uncompressed size

	if (newSize)
		*newSize = compSize + sizeof(uint64_t);

	return compData;
}

void* ndtf_zLibDecompressData(const void* data, size_t size, size_t* newSize)
{
	if (size < sizeof(uint64_t)) return NULL;

	uint64_t uncompSize = *(uint64_t*)data;

	uint8_t* uncompData = (uint8_t*)malloc(uncompSize);
	if (!uncompData) return NULL;

	const void* compData = (uint8_t*)data + sizeof(uint64_t);
	uint64_t compSize = size - sizeof(uint64_t);

	int result = uncompress(uncompData, &uncompSize, compData, compSize);
	if (result != Z_OK)
	{
		free(uncompData);
		return NULL;
	}

	if (newSize)
		*newSize = uncompSize;

	return uncompData;
}


size_t ndtf_file_getDataSize(NDTF_File* file)
{
	size_t totalTexels = 1;
	for (int i = 0; i < file->header.dimensions; i++)
		totalTexels *= file->header.size[i];
	size_t bpp = ndtf_getTexelSize((NDTF_TexelFormat)file->header.texelFormat);
	return totalTexels * bpp;
}

void ndtf_file_free(NDTF_File* file)
{
	if (ndtf_file_isValid(file))
	{
		free(file->data);
		file->header.width = 0;
		file->header.height = 0;
		file->header.depth = 0;
		file->header.ind = 0;
		file->header.ind2 = 0;
	}
}

typedef uint32_t GLenum;

#define GL_RGBA32UI 0x8D70
#define GL_RGB32UI 0x8D71
#define GL_RGBA16UI 0x8D76
#define GL_RGB16UI 0x8D77
#define GL_RGBA8UI 0x8D7C
#define GL_RGBA8 0x8058
#define GL_RGBA16 0x805B
#define GL_RGB8 0x8051
#define GL_RGB16 0x8054
#define GL_RGB8UI 0x8D7D
#define GL_RGBA32F 0x8814
#define GL_RGB32F 0x8815
#define GL_RGBA16F 0x881A
#define GL_RGB16F 0x881B
#define GL_R8 0x8229
#define GL_R16 0x822A
#define GL_R32UI 0x8236
#define GL_R32F 0x822E
#define GL_RED 0x1903
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
GLenum ndtf_glSizedTexelFormat(NDTF_TexelFormat texelFormat)
{
	switch (texelFormat)
	{
	default:
	case NDTF_TEXELFORMAT_RGBA8888:
		return GL_RGBA8;
	case NDTF_TEXELFORMAT_RGBA16161616:
		return GL_RGBA16;
	case NDTF_TEXELFORMAT_RGBA32323232:
		return GL_RGBA32UI;
	case NDTF_TEXELFORMAT_RGBA32323232F:
		return GL_RGBA32F;
	case NDTF_TEXELFORMAT_RGB888:
		return GL_RGB8;
	case NDTF_TEXELFORMAT_RGB161616:
		return GL_RGB16;
	case NDTF_TEXELFORMAT_RGB323232:
		return GL_RGB32UI;
	case NDTF_TEXELFORMAT_RGB323232F:
		return GL_RGB32F;
	case NDTF_TEXELFORMAT_R8:
		return GL_R8;
	case NDTF_TEXELFORMAT_R16:
		return GL_R16;
	case NDTF_TEXELFORMAT_R32:
		return GL_R32UI;
	case NDTF_TEXELFORMAT_R32F:
		return GL_R32F;
	}
}

GLenum ndtf_glTexelFormat(NDTF_TexelFormat texelFormat)
{
	switch (texelFormat)
	{
	default:
	case NDTF_TEXELFORMAT_RGBA8888:
	case NDTF_TEXELFORMAT_RGBA16161616:
	case NDTF_TEXELFORMAT_RGBA32323232:
	case NDTF_TEXELFORMAT_RGBA32323232F:
		return GL_RGBA;
	case NDTF_TEXELFORMAT_RGB888:
	case NDTF_TEXELFORMAT_RGB161616:
	case NDTF_TEXELFORMAT_RGB323232:
	case NDTF_TEXELFORMAT_RGB323232F:
		return GL_RGB;
	case NDTF_TEXELFORMAT_R8:
	case NDTF_TEXELFORMAT_R16:
	case NDTF_TEXELFORMAT_R32:
	case NDTF_TEXELFORMAT_R32F:
		return GL_RED;
	}
}

GLenum ndtf_glTexelType(NDTF_TexelFormat texelFormat)
{
	switch (texelFormat)
	{
	default:
	case NDTF_TEXELFORMAT_RGBA8888:
	case NDTF_TEXELFORMAT_RGB888:
	case NDTF_TEXELFORMAT_R8:
		return GL_UNSIGNED_BYTE;
	case NDTF_TEXELFORMAT_RGBA16161616:
	case NDTF_TEXELFORMAT_RGB161616:
	case NDTF_TEXELFORMAT_R16:
		return GL_UNSIGNED_SHORT;
	case NDTF_TEXELFORMAT_RGBA32323232:
	case NDTF_TEXELFORMAT_RGB323232:
	case NDTF_TEXELFORMAT_R32:
		return GL_UNSIGNED_INT;
	case NDTF_TEXELFORMAT_RGBA32323232F:
	case NDTF_TEXELFORMAT_RGB323232F:
	case NDTF_TEXELFORMAT_R32F:
		return GL_FLOAT;
	}
}
