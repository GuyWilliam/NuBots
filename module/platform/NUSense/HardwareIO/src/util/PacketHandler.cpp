/*
 * MIT License
 *
 * Copyright (c) 2024 NUbots
 *
 * This file is part of the NUbots codebase.
 * See https://github.com/NUbots/NUbots for further info.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "PacketHandler.hpp"

namespace module::platform::NUSense {

    bool PacketHandler::handle() {

        rx_count += 1;

        // Check if we have a header and if we do extract our lengths and pb bytes
        if ((rx_buffer.data[rx_buffer.front] == 0xE2) && (rx_buffer.data[(rx_buffer.front + 1) % RX_BUF_SIZE] == 0x98)
            && (rx_buffer.data[(rx_buffer.front + 2) % RX_BUF_SIZE] == 0xA2)) {

            pb_length = static_cast<uint16_t>(rx_buffer.data[(rx_buffer.front + 3) % RX_BUF_SIZE] << 8)
                        | static_cast<uint16_t>(rx_buffer.data[(rx_buffer.front + 4) % RX_BUF_SIZE]);

            // If the overall packet, including the header, is smaller than
            // the current size of the buffer, then pop all of the payload.
            if ((pb_length + 5) <= rx_buffer.size) {
                pop((uint8_t*) pb_packets, pb_length, 5);
                is_packet_ready = true;
            }
            // Else, work out what the remaining length is, that is the
            // payload's length minus the buffer's size, excluding the
            // five bytes of header.
            else {
                remaining_length = pb_length - rx_buffer.size + 5;
                pop((uint8_t*) pb_packets, rx_buffer.size - 5, 5);
            }
        }
        else if ((remaining_length != 0) /*&& (rx_buffer.size >= remaining_length)*/) {
            uint16_t old_size = pop((uint8_t*) &pb_packets[pb_length - remaining_length],
                                    remaining_length <= rx_buffer.size ? remaining_length : rx_buffer.size,
                                    0);
            remaining_length -= old_size;
            if (remaining_length == 0) {
                is_packet_ready = true;
            }
        }
        else if (rx_buffer.size != 0) {
            // Update index accessor after receiving a packet, making sure to wrap around
            // in case it exceeds the buffer's length
            rx_buffer.front = (rx_buffer.front + 1) % RX_BUF_SIZE;
            rx_buffer.size--;
        }

        if (is_packet_ready) {
            is_packet_ready = false;
            //  Decoding time, deserialise the protocol buffer
            typename NUSense::protobuf_type pb;
            bool stat = pb.ParseFromArray(&pb_packets[0], pb_length);
            nusense   = pb;

            // Monitor the frequency of decodings and missing targets.
            // decode_count += 1;
            // if (targets.targets[0].id != near_id)
            //     near_id = targets.targets[0].id;
            // near_id++;
            // if (targets.targets_count != 20)
            //     missing_count++;
            // if (near_id == 100)
            //     near_id = 100;

            // If stat is false here this means that we failed to decode the packet
            return stat;
        }

        return false;
    }

    const NUSense& PacketHandler::get_nusense_message() {
        return nusense;
    }

    void PacketHandler::receive(const uint32_t& len, uint8_t* buf) {
        // Move the back backwards (higher) in the array unless there is no more room left.
        if (rx_buffer.size > RX_BUF_SIZE) {
            return;
        }

        // If the max buffer size is exceeded, wrap around using 2 copy calls
        if (rx_buffer.back + len > RX_BUF_SIZE) {
            std::copy(&buf[0], &buf[RX_BUF_SIZE - rx_buffer.back], &rx_buffer.data[rx_buffer.back]);
            std::copy(&buf[RX_BUF_SIZE - rx_buffer.back], &buf[len], &rx_buffer.data[0]);
        }
        // If not then 1 copy call should suffice
        else {
            std::copy(&buf[0], &buf[len], &rx_buffer.data[rx_buffer.back]);
        }

        rx_buffer.back = (rx_buffer.back + len) % RX_BUF_SIZE;

        if ((rx_buffer.size + len) >= RX_BUF_SIZE) {
            rx_buffer.size  = RX_BUF_SIZE;
            rx_buffer.front = rx_buffer.back;
        }
        else {
            rx_buffer.size += len;
        }
    }


    uint16_t PacketHandler::pop(uint8_t* bytes, uint16_t length, uint16_t offset) {
        // Update the front to move back (higher) in the array unless there
        // is nothing left in the buffer.
        if (rx_buffer.size < (length + offset)) {
            return length;
        }

        // If the bytes to be popped span across No Man's Land, then use two distinct
        // copies.
        if (((rx_buffer.front + length + offset) >= RX_BUF_SIZE) && ((rx_buffer.front + offset) < RX_BUF_SIZE)) {
            std::copy(&rx_buffer.data[(rx_buffer.front + offset) % RX_BUF_SIZE],
                      &rx_buffer.data[RX_BUF_SIZE],
                      &bytes[0]);
            std::copy(&rx_buffer.data[0],
                      &rx_buffer.data[length - RX_BUF_SIZE + rx_buffer.front + offset],
                      &bytes[RX_BUF_SIZE - rx_buffer.front - offset]);
        }
        // Else, use one straightforward copy.
        else {
            std::copy(&rx_buffer.data[(rx_buffer.front + offset) % RX_BUF_SIZE],
                      &rx_buffer.data[(rx_buffer.front + offset + length) % RX_BUF_SIZE],
                      &bytes[0]);
        }
        // Move the front forward and decrease the size.
        rx_buffer.front = (rx_buffer.front + length + offset) % RX_BUF_SIZE;
        rx_buffer.size -= length + offset;

        return length;
    }

}  // namespace module::platform::NUSense
