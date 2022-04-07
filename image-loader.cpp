#include "image-loader.h"

#include <vector>
#include <string>
#include <exception>
#include <cstring>

typedef void* tjhandle;

#ifdef __cplusplus
extern "C" {
#endif

tjhandle tjInitDecompress(void);

tjhandle tjInitCompress(void);

int tjDestroy(tjhandle);

const char *tjGetErrorStr2(tjhandle);

int tjDecompressWithCropping(tjhandle handle, const unsigned char *jpegBuf,
                                        unsigned long jpegSize, unsigned char *dstBuf,
                                        int width, int pitch, int height, int cropX,
                                        int cropY, int pixelFormat, int flags);

int tjCompress2(tjhandle handle, const unsigned char *srcBuf,
                           int width, int pitch, int height, int pixelFormat,
                           unsigned char **jpegBuf, unsigned long *jpegSize,
                           int jpegSubsamp, int jpegQual, int flags);

IMAGE_LOADER_API int decompress_image(Image *image, const uint8_t *data, size_t size, CropRect *rect)
{
    tjhandle handle = tjInitDecompress();
    if (!handle)
    {
        throw("Unable to init decompression context");
    }

    image->request_memory(rect->w, rect->h, 4);

    int padding = rect->x & 7;
    int err = tjDecompressWithCropping(
        handle,
        data,
        size,
        image->buffer(),
        image->Width + padding,
        0,
        image->Height,
        rect->x - padding,
        rect->y,
        PixelFormat::RGBA, 0
        );

    if (err)
    {
        auto error = tjGetErrorStr2(handle);
        printf("Error: %s\n", error);
        tjDestroy(handle);
        throw error;
    }

    tjDestroy(handle);

    if (padding > 0)
    {
        auto rgba_buffer = image->buffer();
        int pixel_size   = 4;
        int offset       = padding * pixel_size;
        int src_pitch    = rect->w * pixel_size;
        int dst_pitch    = src_pitch + offset;
        for (int i = 0; i < rect->h; i++)
        {
            memcpy(rgba_buffer + i * src_pitch, rgba_buffer + i * dst_pitch + offset, src_pitch);
        }
    }

    return DECOMPRESS_SUCCESS;
}

IMAGE_LOADER_API int compress_image(uint8_t **pData, size_t *size_, Image *image)
{
    tjhandle handle = tjInitCompress();
    if (!handle)
    {
        throw("Unable to init decompression context");
    }

    unsigned long size = *size_;

    int err = tjCompress2(handle,
        image->buffer(),
        image->Width,
        0,
        image->Height,
        PixelFormat::RGBA,
        pData,
        &size,
        1, 100, 0
        );

    *size_ = size;
    if (err)
    {
        auto error = tjGetErrorStr2(handle);
        printf("Error: %s\n", error);
        tjDestroy(handle);
        throw error;
    }

    tjDestroy(handle);

    return COMPRESS_SUCCESS;
}

#ifdef __cplusplus
}
#endif
