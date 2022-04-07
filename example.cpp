#include "image-loader.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>

using PFN_decompress_image = int (*)(Image *image, const uint8_t *data, size_t size, CropRect *rect);
using PFN_compress_image   = int (*)(uint8_t **pData, size_t *size, Image *image);

class DLLController
{
public:
    virtual ~DLLController() = default;

    virtual void *get_process_address(const std::string &func_name) = 0;

    virtual bool is_available() const = 0;

    template <class T>
    T get_func(const std::string &func_name)
    {
        return reinterpret_cast<T>(get_process_address(func_name));
    }
};

#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
class Win32DLLController : public DLLController
{
public:
    Win32DLLController(const std::string &path)
    {
        handle = LoadLibraryA(path.c_str());
    }

    virtual ~Win32DLLController()
    {
        FreeLibrary(handle);
    }

    virtual void *get_process_address(const std::string &func_name) override
    {
        return GetProcAddress(handle, func_name.c_str());
    }

    virtual bool is_available() const override
    {
        return !!handle;
    }

protected:
    HMODULE handle = nullptr;
};
#endif

#ifdef __linux__
#define _UNIX03_SOURCE
#include <dlfcn.h>
class UnixDLLController : public DLLController
{
public:
    UnixDLLController(const std::string &path)
    {
        handle = dlopen(path.c_str(), RTLD_LAZY);
    }

    virtual ~UnixDLLController()
    {
        dlclose(handle);
    }

    virtual void *get_process_address(const std::string &func_name) override
    {
        return dlsym(handle, func_name.c_str());
    }

    virtual bool is_available() const override
    {
        return !!handle;
    }

protected:
    void *handle = nullptr;
};
#endif

static std::vector<uint8_t> load_image_from_file(const std::string &path)
{
    FILE *fp = fopen(path.c_str(), "rb+");
    if (!fp)
    {
        throw std::runtime_error("No such file: " + path);
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);

    std::vector<uint8_t> raw_jpeg_data;
    raw_jpeg_data.resize(size);
    fread(raw_jpeg_data.data(), 1, size, fp);

    fclose(fp);

    return raw_jpeg_data;
}

static void save_file(const std::string &path, uint8_t *data, size_t size)
{
    FILE *fp = fopen(path.c_str(), "wb+");
    if (!fp)
    {
        throw std::runtime_error("Corrupted: " + path);
    }

    fwrite(data, size, 1, fp);

    fclose(fp);
}

int main()
{
#ifdef _WIN32
    auto dll_controller = Win32DLLController("./image-loader.dll");
#else
    auto dll_controller = UnixDLLController("/home/qsxw/C/image-loader/build/libimage-loader.so");
#endif

    if (!dll_controller.is_available())
    {
        throw std::runtime_error("Unable to open the dynamic library file specified!");
    }

    auto decompress_image = dll_controller.get_func<PFN_decompress_image>("decompress_image");
    auto compress_image   = dll_controller.get_func<PFN_compress_image>("compress_image");

    Image image{};
    CropRect rect{ 100, 100, 0, 0 };

    auto jpeg = load_image_from_file("../Assets/4k_sam.jpg");

    decompress_image(&image, jpeg.data(), jpeg.size(), &rect);

    uint8_t *buf = nullptr;
    size_t size = 0;

    compress_image(&buf, &size, &image);

    save_file("test.jpg", buf, size);
    return 0;
}
