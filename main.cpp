/**
 * pngcmp main
 *
 * @author AKIYAMA Kouhei
 * @since 2017-02-02
 */

#ifndef PNGCMP_LINK_STATIC_LIBRARY
# define PNGCMP_LINK_STATIC_LIBRARY 1
#endif
#if PNGCMP_LINK_STATIC_LIBRARY
# ifdef _DEBUG
#  pragma comment(lib, "Debug Library/libpng16.lib")
#  pragma comment(lib, "ZlibStatDebug/zlibstat.lib")
# else
#  pragma comment(lib, "Release Library/libpng16.lib")
#  pragma comment(lib, "ZlibStatRelease/zlibstat.lib")
# endif
#endif

#include <iostream>
#include <string>
#include <cstdio>
#include <setjmp.h>
#include <png.h>

struct PngStruct
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    PngStruct()
        : png_ptr(png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr))
        , info_ptr(png_ptr ? png_create_info_struct(png_ptr) : nullptr)
        , end_info(png_ptr ? png_create_info_struct(png_ptr) : nullptr)
    {
    }
    ~PngStruct()
    {
        if(png_ptr){
            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        }
    }
    bool hasError() const
    {
        return !png_ptr || !info_ptr || !end_info;
    }
};

struct File
{
    FILE *fp;
    File(const char *filename) : fp(std::fopen(filename, "rb")){}
    ~File(){if(fp){std::fclose(fp);}}
    operator FILE *() const {return fp;}
};

class PngFile
{
    std::string filename;
    File fp;
    PngStruct png;
public:
    PngFile(const char * const filename)
        : filename(filename)
        , fp(filename)
        , png()
    {
    }
    void error(const std::string &msg)
    {
        std::cerr << filename << ":: " << msg << std::endl;
    }
    bool read()
    {
        if(!fp){
            error("File cannot open.");
            return false;
        }
        if(png.hasError()){
            error("Failed to initialize libpng.");
            return false;
        }

        png_init_io(png.png_ptr, fp);

        // read image.
        int transform
            = PNG_TRANSFORM_STRIP_16
            | PNG_TRANSFORM_EXPAND
            | PNG_TRANSFORM_BGR;

        png_read_png(png.png_ptr, png.info_ptr, transform, NULL);

        const int color_type = png_get_color_type(png.png_ptr, png.info_ptr);
        const int bit_depth = png_get_bit_depth(png.png_ptr, png.info_ptr);

        const std::size_t pixelBytes =
            color_type == PNG_COLOR_TYPE_RGB ? 3 :
            color_type == PNG_COLOR_TYPE_RGB_ALPHA ? 4 :
            0;
        if (pixelBytes == 0){
            error("unsupported color type found.");
            return false;
        }
        ///@todo 16bit-depth
        ///@todo PNG_COLOR_TYPE_PALETTE
        return true;
    }
    const png_bytep *rows(){return png_get_rows(png.png_ptr, png.info_ptr);}
    png_uint_32 width(){return png_get_image_width(png.png_ptr, png.info_ptr);}
    png_uint_32 height(){return png_get_image_height(png.png_ptr, png.info_ptr);}
    int color_type(){return png_get_color_type(png.png_ptr, png.info_ptr);}
};



int main(int argc, char *argv[])
{
    if(argc < 3){
        std::cout << "usage: " << argv[0] << " <png filename1> <png filename2>" << std::endl;
        return -1;
    }

	// read png
    PngFile img1(argv[1]);
    if(!img1.read()){return -1;}
    PngFile img2(argv[2]);
    if(!img2.read()){return -1;}

	// compare width, height, color_type
    if(img1.width() != img2.width() || img1.height() != img2.height()){
        std::cerr << "Image size mismatch." << std::endl;
        return -1;
    }
    if(img1.color_type() != img2.color_type()){
        std::cerr << "Image color type mismatch." << std::endl;
        return -1;
    }

	// compare pixels
    const unsigned int width = img1.width();
    const unsigned int height = img1.height();
    const int color_type = img1.color_type();
    const png_bytep * const rows1 = img1.rows();
    const png_bytep * const rows2 = img2.rows();

    int error_count = 0;
    for(unsigned int y = 0; y < height; ++y){
        png_bytep p1 = rows1[y];
        png_bytep p2 = rows2[y];

        if(color_type == PNG_COLOR_TYPE_RGB){
            for(unsigned int x = 0; x < width; ++x){
                if(p1[0] != p2[0] ||
                   p1[1] != p2[1] ||
                   p1[2] != p2[2]){
                    ++error_count;
                }
                p1 += 3;
                p2 += 3;
            }
        }
        else if(color_type == PNG_COLOR_TYPE_RGB_ALPHA){
            for(unsigned int x = 0; x < width; ++x){
                if(p1[0] != p2[0] ||
                   p1[1] != p2[1] ||
                   p1[2] != p2[2] ||
                   p1[3] != p2[3]){
                    ++error_count;
                }
                p1 += 4;
                p2 += 4;
            }
        }
        else{
            return -1;
        }
    }

	// output result
    const unsigned int total = width * height;
    const unsigned int matched = total - error_count;
    std::cout << matched << "\t" << total << std::endl;
    //std::cout << (100 * matched / total) << std::endl;

    return 0;
}
