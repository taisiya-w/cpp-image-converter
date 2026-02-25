#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>
#include <cstdint>
#include <vector>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t bfType;      // 'BM'
    uint32_t bfSize;       // размер файла
    uint16_t bfReserved1;  // 0
    uint16_t bfReserved2;  // 0
    uint32_t bfOffBits;    // смещение данных от начала файла
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t biSize;          // размер этого заголовка = 40
    int32_t  biWidth;         // ширина
    int32_t  biHeight;        // высота (положительная - снизу вверх)
    uint16_t biPlanes;        // количество плоскостей = 1
    uint16_t biBitCount;      // бит на пиксель = 24
    uint32_t biCompression;   // тип сжатия = 0 (без сжатия)
    uint32_t biSizeImage;     // размер данных (stride * height)
    int32_t  biXPelsPerMeter; // горизонтальное разрешение = 11811
    int32_t  biYPelsPerMeter; // вертикальное разрешение = 11811
    uint32_t biClrUsed;       // количество использованных цветов = 0
    uint32_t biClrImportant;  // количество значимых цветов = 0x1000000
}
PACKED_STRUCT_END

static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out) {
        return false;
    }

    int w = image.GetWidth();
    int h = image.GetHeight();
    int stride = GetBMPStride(w);
    int dataSize = stride * h;
    int fileSize = 54 + dataSize;

    BitmapFileHeader fileHeader;
    fileHeader.bfType = 0x4D42;
    fileHeader.bfSize = fileSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = 54;

    BitmapInfoHeader infoHeader;
    infoHeader.biSize = 40;
    infoHeader.biWidth = w;
    infoHeader.biHeight = h;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = dataSize;
    infoHeader.biXPelsPerMeter = 11811;
    infoHeader.biYPelsPerMeter = 11811;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0x1000000;

    out.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    out.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    vector<char> buf(stride, 0);

    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            buf[x * 3]     = static_cast<char>(line[x].b);
            buf[x * 3 + 1] = static_cast<char>(line[x].g);
            buf[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buf.data(), stride);
        if (!out) {
            return false;
        }
    }

    return true;
}

Image LoadBMP(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) {
        return {};
    }

    BitmapFileHeader fileHeader;
    in.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    if (!in || fileHeader.bfType != 0x4D42) {
        return {};
    }

    BitmapInfoHeader infoHeader;
    in.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    if (!in) {
        return {};
    }

    if (infoHeader.biSize != 40 ||
        infoHeader.biPlanes != 1 ||
        infoHeader.biBitCount != 24 ||
        infoHeader.biCompression != 0) {
        return {};
    }

    int w = infoHeader.biWidth;
    int h = infoHeader.biHeight;
    if (w <= 0 || h <= 0) {
        return {};
    }

    int stride = GetBMPStride(w);
    if (infoHeader.biSizeImage != stride * h) {
        return {};
    }

    Image result(w, h, Color::Black());
    vector<char> buf(stride);

    for (int y = h - 1; y >= 0; --y) {
        in.read(buf.data(), stride);
        if (!in) {
            return {};
        }

        Color* line = result.GetLine(y);
        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buf[x * 3]);
            line[x].g = static_cast<byte>(buf[x * 3 + 1]);
            line[x].r = static_cast<byte>(buf[x * 3 + 2]);
        }
    }

    return result;
}

} // namespace img_lib