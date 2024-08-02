/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <b2nd.h>
#include <blosc2.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <xiApi.h>

#include <chrono>
#include <msgpack.hpp>

#include "src/util.h"

#define HandleBLOSCResult(res, place)                                                                                  \
    if (res != 0)                                                                                                      \
    {                                                                                                                  \
        std::stringstream errormsg;                                                                                    \
        errormsg << "Error after " << place << " " << res << "\n";                                                     \
        throw std::runtime_error(errormsg.str());                                                                      \
    }

void CreateBLOSCArray(char *urlpath)
{
    blosc2_init();
    auto *image = new XI_IMG;
    int width = 4 * 512;
    int height = 4 * 272;
    image->width = width;
    image->height = height;
    image->bp = reinterpret_cast<void *>(new uint16_t[image->width * image->height]());
    int N_images = 10;

    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    cparams.typesize = sizeof(uint16_t);
    cparams.compcode = BLOSC_ZSTD;
    cparams.filters[BLOSC2_MAX_FILTERS - 2] = BLOSC_BITSHUFFLE;
    cparams.filters[BLOSC2_MAX_FILTERS - 1] = BLOSC_SHUFFLE;
    cparams.clevel = 1;
    cparams.nthreads = 4;

    blosc2_storage storage = BLOSC2_STORAGE_DEFAULTS;
    storage.contiguous = true;
    storage.cparams = &cparams;
    storage.urlpath = urlpath;

    // Shape of the ndarray
    int64_t shape[] = {0, image->height, image->width};
    // Set Chunk shape and BlockShape as per your requirement.
    int32_t chunk_shape[] = {1, height, width};
    int32_t block_shape[] = {1, height, width};

    b2nd_context_t *ctx =
        b2nd_create_ctx(&storage, 3, shape, chunk_shape, block_shape, "|u2", DTYPE_NUMPY_FORMAT, nullptr, 0);

    b2nd_array_t *src;
    int result;
    if (access(urlpath, F_OK) != -1)
    {
        result = b2nd_open(urlpath, &src);
        printf("Opened existing file\n");
    }
    else
    {
        result = b2nd_empty(ctx, &src);
        printf("Created non-existent file\n");
    }
    HandleBLOSCResult(result, "b2nd_empty || b2nd_open");
    HandleBLOSCResult(result, "b2nd_empty");
    // Determine the buffer size (in bytes)
    const int64_t buffer_size = width * height * sizeof(uint16_t);
    // loop through all images
    double total_time = 0;
    for (int i = 0; i < N_images; i++)
    {
        printf("Saving image #: %d\n", i);
        // Generate random image data
        for (int j = 0; j < (image->width * image->height); j++)
        {
            // generate random values: extreme case where compression is very
            // difficult
            reinterpret_cast<uint16_t *>(image->bp)[j] = rand() % 65535;
        }

        auto start_time = std::chrono::high_resolution_clock::now();
        int result = b2nd_append(src, image->bp, buffer_size, 0);
        auto end_time = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = end_time - start_time;
        total_time += elapsed.count();
        HandleBLOSCResult(result, "b2nd_append");
    }
    std::cout << "Total time spent with b2nd_append: " << total_time << " seconds\n";

    // And for the fwrite method
    auto start_time = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < N_images; i++)
    {
        auto file = fopen("test_image.dat", "wb");
        fwrite(image->bp, image->width * image->height, sizeof(uint16_t), file);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    std::cout << "Time spent with fwrite: " << elapsed.count() << " seconds\n";
    std::remove("test_image.dat");

    // create an array and pack it.
    std::vector<int> intArray = {1, 2, 3, 4, 5};
    msgpack::sbuffer sbufInt;
    msgpack::pack(sbufInt, intArray);
    AppendBLOSCVLMetadata(src, "intMetadata", sbufInt);

    // create array of strings and pack it
    std::vector<std::string> stringArray = {"one", "two", "three", "four", "five"};
    msgpack::sbuffer sbufString;
    msgpack::pack(sbufString, stringArray);
    AppendBLOSCVLMetadata(src, "stringMetadata", sbufString);

    b2nd_free(src);
    b2nd_free_ctx(ctx);
    blosc2_destroy();
    delete[] reinterpret_cast<uint16_t *>(image->bp);
    delete image;
}

TEST(BLOSC, BloscAppend)
{
    char *urlpath = strdup("test_image_dataset.b2nd");
    blosc2_remove_urlpath(urlpath);
    CreateBLOSCArray(urlpath);
    blosc2_remove_urlpath(urlpath);
}

TEST(BLOSC, BloscAppendToExistingFile)
{
    char *urlpath = strdup("test_image_dataset.b2nd");
    blosc2_remove_urlpath(urlpath);
    CreateBLOSCArray(urlpath);
    CreateBLOSCArray(urlpath);
    blosc2_remove_urlpath(urlpath);
}
