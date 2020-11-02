#include "Factory.h"

namespace module::input::decompressor::turbojpeg {

Factory::Factory() {}
Factory::~Factory() {}

std::shared_ptr<decompressor::Decompressor> Factory::make_decompressor(const uint32_t& width,
                                                                       const uint32_t& height,
                                                                       const uint32_t& format) {
    return std::make_shared<turbojpeg::Decompressor>(width, height, format);
}

}  // namespace module::input::decompressor::turbojpeg
