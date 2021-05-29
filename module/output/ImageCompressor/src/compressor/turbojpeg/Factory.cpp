#include "Factory.hpp"

namespace module::output::compressor::turbojpeg {

    Factory::Factory(const int& qty) : quality(qty) {}
    Factory::~Factory() = default;

    std::shared_ptr<compressor::Compressor> Factory::make_compressor(const uint32_t& width,
                                                                     const uint32_t& height,
                                                                     const uint32_t& format) {
        return std::make_shared<turbojpeg::Compressor>(quality, width, height, format);
    }

}  // namespace module::output::compressor::turbojpeg
