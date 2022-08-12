#ifndef MESSAGE_FORMAT_MATRIX_TYPES_HPP
#define MESSAGE_FORMAT_MATRIX_TYPES_HPP
#include <Eigen/Core>
#include <fmt/ostream.h>

namespace fmt {

    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 3, 1>>> : ostream_formatter {};

    template <> struct formatter<Eigen::Matrix<double, Eigen::Dynamic, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, Eigen::Dynamic, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, Eigen::Dynamic, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<uint8_t, Eigen::Dynamic, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 2, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 2, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 2, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 2, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 3, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 3, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 3, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 3, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 4, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 4, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 4, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 4, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 5, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 5, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 5, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 5, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 6, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 6, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 6, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 6, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 7, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 7, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 7, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 7, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 8, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 8, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 8, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 8, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 9, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 9, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 9, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 9, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 10, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 10, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 10, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 10, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 11, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 11, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 11, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 11, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 12, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 12, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 12, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 12, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 13, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 13, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 13, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 13, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 14, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 14, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 14, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 14, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 15, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 15, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 15, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 15, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 16, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 16, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 16, 1>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 16, 1>> : ostream_formatter {};

    template <> struct formatter<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, Eigen::Dynamic, Eigen::Dynamic>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 2, 2>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 2, 2>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 2, 2>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 2, 2>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 3, 3>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 3, 3>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 3, 3>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 3, 3>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 4, 4>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 4, 4>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 4, 4>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 4, 4>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 5, 5>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 5, 5>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 5, 5>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 5, 5>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 6, 6>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 6, 6>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 6, 6>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 6, 6>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 7, 7>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 7, 7>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 7, 7>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 7, 7>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 8, 8>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 8, 8>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 8, 8>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 8, 8>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 9, 9>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 9, 9>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 9, 9>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 9, 9>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 10, 10>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 10, 10>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 10, 10>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 10, 10>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 11, 11>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 11, 11>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 11, 11>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 11, 11>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 12, 12>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 12, 12>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 12, 12>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 12, 12>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 13, 13>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 13, 13>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 13, 13>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 13, 13>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 14, 14>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 14, 14>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 14, 14>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 14, 14>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 15, 15>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 15, 15>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 15, 15>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 15, 15>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<double, 16, 16>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<float, 16, 16>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<int, 16, 16>> : ostream_formatter {};
    template <> struct formatter<Eigen::Matrix<unsigned int, 16, 16>> : ostream_formatter {};

    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, Eigen::Dynamic, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, Eigen::Dynamic, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, Eigen::Dynamic, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<uint8_t, Eigen::Dynamic, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 2, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 2, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 2, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 2, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 3, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 3, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 3, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 3, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 4, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 4, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 4, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 4, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 5, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 5, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 5, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 5, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 6, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 6, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 6, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 6, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 7, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 7, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 7, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 7, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 8, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 8, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 8, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 8, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 9, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 9, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 9, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 9, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 10, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 10, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 10, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 10, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 11, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 11, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 11, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 11, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 12, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 12, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 12, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 12, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 13, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 13, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 13, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 13, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 14, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 14, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 14, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 14, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 15, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 15, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 15, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 15, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 16, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 16, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 16, 1>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 16, 1>>> : ostream_formatter {};

    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, Eigen::Dynamic, Eigen::Dynamic>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 2, 2>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 2, 2>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 2, 2>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 2, 2>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 3, 3>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 3, 3>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 3, 3>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 3, 3>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 4, 4>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 4, 4>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 4, 4>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 4, 4>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 5, 5>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 5, 5>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 5, 5>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 5, 5>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 6, 6>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 6, 6>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 6, 6>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 6, 6>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 7, 7>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 7, 7>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 7, 7>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 7, 7>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 8, 8>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 8, 8>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 8, 8>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 8, 8>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 9, 9>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 9, 9>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 9, 9>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 9, 9>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 10, 10>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 10, 10>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 10, 10>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 10, 10>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 11, 11>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 11, 11>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 11, 11>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 11, 11>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 12, 12>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 12, 12>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 12, 12>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 12, 12>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 13, 13>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 13, 13>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 13, 13>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 13, 13>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 14, 14>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 14, 14>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 14, 14>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 14, 14>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 15, 15>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 15, 15>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 15, 15>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 15, 15>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<double, 16, 16>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<float, 16, 16>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<int, 16, 16>>> : ostream_formatter {};
    template <> struct formatter<Eigen::Transpose<Eigen::Matrix<unsigned int, 16, 16>>> : ostream_formatter {};

} // namespace fmt

#endif // MESSAGE_FORMAT_MATRIX_TYPES_HPP
