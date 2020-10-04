#include "Decompressor.h"

#include <fmt/format.h>
#include <turbojpeg.h>

#include "utility/vision/fourcc.h"

namespace module::input::decompressor::turbojpeg {

uint32_t decompressed_format(const uint32_t& format) {
    using utility::vision::fourcc;

    switch (format) {
        case fourcc("JPBG"): return fourcc("BGGR");  // Permuted Bayer
        case fourcc("JPRG"): return fourcc("RGGB");  // Permuted Bayer
        case fourcc("JPGR"): return fourcc("GRBG");  // Permuted Bayer
        case fourcc("JPGB"): return fourcc("GBRG");  // Permuted Bayer

        case fourcc("PJBG"): return fourcc("PBG8");  // Polarized Colour
        case fourcc("PJRG"): return fourcc("PRG8");  // Polarized Colour
        case fourcc("PJGR"): return fourcc("PGR8");  // Polarized Colour
        case fourcc("PJGB"): return fourcc("PGB8");  // Polarized Colour

        case fourcc("PJPG"): return fourcc("PY8 ");  // Polarized Monochrome

        case fourcc("JPEG"): return fourcc("JPEG");  // Could be either grey or colour jpeg, work out later

        default: throw std::runtime_error("Unhandled format");
    }
}

Decompressor::Decompressor(const uint32_t& width, const uint32_t& height, const uint32_t& format)
    : width(width), height(height), format(format), output_fourcc(decompressed_format(format)) {

    // If this is a mosaic format, build a mosaic table
    if (utility::vision::Mosaic::size(output_fourcc) > 0) {
        mosaic = utility::vision::Mosaic(width, height, output_fourcc);
    }
}
Decompressor::~Decompressor() {}

std::pair<std::vector<uint8_t>, int> Decompressor::decompress(const std::vector<uint8_t>& data) {

    static thread_local tjhandle decompressor = tjInitDecompress();

    // Read the header
    int width;
    int height;
    int subsamp;
    tjDecompressHeader2(decompressor, const_cast<uint8_t*>(data.data()), data.size(), &width, &height, &subsamp);

    // Work out what we decode as
    TJPF code = mosaic || subsamp == TJSAMP::TJSAMP_GRAY ? TJPF::TJPF_GRAY : TJPF::TJPF_RGB;

    // Decode the image
    std::vector<uint8_t> output(width * height * (code == TJPF::TJPF_GRAY ? 1 : 3));
    tjDecompress2(decompressor,
                  data.data(),
                  data.size(),
                  output.data(),
                  width,
                  0 /*pitch*/,
                  height,
                  code,
                  TJFLAG_FASTDCT);

    // If we were a mosaic then permute it back
    if (mosaic) {
        output = mosaic.unpermute(output);
    }

    // Work out what the fourcc of the image should be
    uint32_t fourcc =
        output_fourcc == utility::vision::fourcc("JPEG")
            ? subsamp == TJSAMP::TJSAMP_GRAY ? utility::vision::fourcc("GREY") : utility::vision::fourcc("RGB3")
            : output_fourcc;

    return std::make_pair(output, fourcc);
}

}  // namespace module::input::decompressor::turbojpeg
