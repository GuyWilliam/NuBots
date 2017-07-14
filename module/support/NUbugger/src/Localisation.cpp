/*
 * This file is part of the NUbots Codebase.
 *
 * The NUbots Codebase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The NUbots Codebase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the NUbots Codebase.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 NUbots <nubots@nubots.net>
 */

#include "NUbugger.h"

#include "message/localisation/FieldObject.h"
#include "message/localisation/Localisation.h"

#include "utility/localisation/transform.h"
#include "utility/nubugger/NUhelpers.h"
#include "utility/support/eigen_armadillo.h"
#include "utility/time/time.h"

namespace module {
namespace support {

    using utility::nubugger::graph;
    using utility::time::getUtcTimestamp;
    using message::localisation::FieldObject;
    using message::localisation::Ball;
    using message::localisation::Self;
    using message::localisation::Localisation;
    using message::localisation::Model;

    void NUbugger::provideLocalisation() {

        handles["localisation"].push_back(
            on<Every<10, Per<std::chrono::seconds>>,
               Optional<With<Self>>,
               Optional<With<Ball>>,
               Single,
               Priority::LOW>()
                .then([this](std::shared_ptr<const Self> self, std::shared_ptr<const Ball> ball) {

                    if (self) send(*self, 0, false, NUClear::clock::now());
                    if (ball) send(*ball, 0, false, NUClear::clock::now());

                }));
    }
}  // namespace support
}  // namespace module
