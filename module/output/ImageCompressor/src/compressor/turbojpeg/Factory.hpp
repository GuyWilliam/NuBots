#ifndef MODULE_OUTPUT_IMAGECOMPRESSOR_COMPRESSOR_TURBOJPEG_FACTORY_HPP
#define MODULE_OUTPUT_IMAGECOMPRESSOR_COMPRESSOR_TURBOJPEG_FACTORY_HPP

#include "../CompressorFactory.hpp"
#include "Compressor.hpp"

namespace module {
namespace output {
    namespace compressor {
        namespace turbojpeg {

            class Factory : public CompressorFactory {
            public:
                Factory(const int& quality);
                virtual ~Factory();

                virtual std::shared_ptr<compressor::Compressor> make_compressor(const uint32_t width,
                                                                                const uint32_t& height,
                                                                                const uint32_t& format) override;

            private:
                /// The quality that this compressor is configured for
                int quality;
            };

        }  // namespace turbojpeg
    }      // namespace compressor
}  // namespace output
}  // namespace module

#endif  //  MODULE_OUTPUT_IMAGECOMPRESSOR_COMPRESSOR_TURBOJPEG_FACTORY_HPP
