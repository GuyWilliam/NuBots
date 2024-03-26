/*
 * MIT License
 *
 * Copyright (c) 2014 NUbots
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

#ifndef MODULES_INPUT_GAMECONTROLLER_HPP
#define MODULES_INPUT_GAMECONTROLLER_HPP

#include <nuclear>

#include "GameControllerData.hpp"

#include "message/input/GameEvents.hpp"
#include "message/input/GameState.hpp"

namespace module::input {

    /**
     * Monitors the match Game Controller
     *
     * @author Brendan Annable
     * @author Jordan Johnson
     * @author Trent Houliston
     */
    class GameController : public NUClear::Reactor {
    private:
        static constexpr const uint SUPPORTED_VERSION       = 12;
        static constexpr const uint PLAYERS_PER_TEAM        = 6;
        static constexpr const uint ACTIVE_PLAYERS_PER_TEAM = 4;
        static constexpr const uint NUM_TEAMS               = 2;

        uint receive_port;
        uint send_port;
        uint TEAM_ID;
        uint PLAYER_ID;
        std::string BROADCAST_IP;
        std::string udp_filter_address;
        std::set<std::string> ignored_ip_addresses;

        bool selfPenalised = true;
        ReactionHandle listenHandle;

        gamecontroller::GameControllerPacket packet;
        gamecontroller::Mode mode;

        void resetState();
        void process(const message::input::GameState& oldGameState,
                     const gamecontroller::GameControllerPacket& oldPacket,
                     const gamecontroller::GameControllerPacket& newPacket);
        void sendReplyMessage(const gamecontroller::ReplyMessage& message);
        [[nodiscard]] const gamecontroller::Team& getOwnTeam(const gamecontroller::GameControllerPacket& state) const;
        [[nodiscard]] const gamecontroller::Team& getOpponentTeam(
            const gamecontroller::GameControllerPacket& state) const;
        [[nodiscard]] static message::input::GameState::Data::PenaltyReason getPenaltyReason(
            const gamecontroller::PenaltyState& penaltyState);

        [[nodiscard]] static std::string ipAddressIntToString(const uint32_t& ipAddr);

    public:
        explicit GameController(std::unique_ptr<NUClear::Environment> environment);
    };
}  // namespace module::input

#endif  // MODULES_INPUT_GAMECONTROLLER_HPP
