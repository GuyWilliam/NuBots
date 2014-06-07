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
 * Copyright 2013 NUBots <nubots@nubots.net>
 */

#include "LUTClassifier.h"

#include <yaml-cpp/yaml.h>

#include "messages/vision/LookUpTable.h"
#include "messages/vision/SaveLookUpTable.h"

namespace modules {
    namespace vision {

        using messages::input::Image;
        using messages::vision::ColourSegment;
        using messages::support::Configuration;
        using messages::vision::ClassifiedImage;
        using messages::vision::SegmentedRegion;
        using messages::vision::LookUpTable;
        using messages::vision::SaveLookUpTable;

        using std::chrono::system_clock;

        LUTClassifier::LUTClassifier(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)), greenHorizon(), scanLines() {

            on<Trigger<Configuration<VisionConstants>>>([this](const Configuration<VisionConstants>&) {
                //std::cout<< "Loading VisionConstants."<<std::endl;
                //std::cout<< "Finished Config Loading successfully."<<std::endl;
            });

            //Load LUTs
            on<Trigger<Configuration<LUTLocations>>>([this](const Configuration<LUTLocations>& config) {
                std::string LUTLocation = config["LUT_LOCATION"];
                emit(std::make_unique<LookUpTable>(LUTLocation));
            });

            on<Trigger<SaveLookUpTable>, With<LookUpTable, Configuration<LUTLocations>>>([this](const SaveLookUpTable&, const LookUpTable& lut, const Configuration<LUTLocations>& config) {
                std::string LUTLocation = config["LUT_LOCATION"];
                lut.save(LUTLocation);
            });

            //Load in greenhorizon parameters
            on<Trigger<Configuration<GreenHorizonConfig>>>([this](const Configuration<GreenHorizonConfig>& constants) {
                //std::cout<< "Loading gh cONFIG."<<std::endl;
                greenHorizon.setParameters(constants.config["GREEN_HORIZON_SCAN_SPACING"],
                                           constants.config["GREEN_HORIZON_MIN_GREEN_PIXELS"],
                                           constants.config["GREEN_HORIZON_UPPER_THRESHOLD_MULT"]);
                //std::cout<< "Finished Config Loading successfully."<<std::endl;
            });

            //Load in scanline parameters
            on<Trigger<Configuration<ScanLinesConfig>>>([this](const Configuration<ScanLinesConfig>& constants) {
                //std::cout<< "Loading ScanLines config."<<std::endl;
                scanLines.setParameters(constants.config["HORIZONTAL_SCANLINE_SPACING"],
                                         constants.config["VERTICAL_SCANLINE_SPACING"],
                                         constants.config["APPROXIMATE_SEGS_PER_HOR_SCAN"],
                                         constants.config["APPROXIMATE_SEGS_PER_VERT_SCAN"]);
                //std::cout<< "Finished Config Loading successfully."<<std::endl;
            });


            on<Trigger<Configuration<RulesConfig>>>([this](const Configuration<RulesConfig>& rules) {
                //std::cout<< "Loading Rules config."<<std::endl;
                segmentFilter.clearRules();
                if(rules.config["USE_REPLACEMENT_RULES"]){

                    for (const auto& rule : rules.config["REPLACEMENT_RULES"]) {
                        //std::cout << "Loading Replacement rule : " << rule.first << std::endl;

                        ColourReplacementRule r;

                        std::vector<unsigned int> before = rule.second["before"]["vec"];
                        std::vector<unsigned int> middle = rule.second["middle"]["vec"];
                        std::vector<unsigned int> after = rule.second["after"]["vec"];

                        r.loadRuleFromConfigInfo(rule.second["before"]["colour"],
                                                rule.second["middle"]["colour"],
                                                rule.second["after"]["colour"],
                                                before[0],//min
                                                before[1],//max, etc.
                                                middle[0],
                                                middle[1],
                                                after[0],
                                                after[1],
                                                rule.second["replacement"]);

                        //Neat method which is broken due to config system
                        /*r.loadRuleFromConfigInfo(rule.second["before"]["colour"],
                                                rule.second["middle"]["colour"],
                                                rule.second["after"]["colour"],
                                                unint_32(rule.second["before"]["vec"][0]),//min
                                                unint_32(rule.second["before"]["vec"][1]),//max, etc.
                                                unint_32(rule.second["middle"]["vec"][0]),
                                                unint_32(rule.second["middle"]["vec"][1]),
                                                unint_32(rule.second["after"]["vec"][0]),
                                                unint_32(rule.second["after"]["vec"][1]),
                                                rule.second["replacement"]);*/
                        segmentFilter.addReplacementRule(r);
                        //std::cout<< "Finished Config Loading successfully."<<std::endl;
                    }
                }

                for(const auto& rule : rules.config["TRANSITION_RULES"]) {
                    //std::cout << "Loading Transition rule : " << rule.first << std::endl;

                    ColourTransitionRule r;

                    std::vector<unsigned int> before = rule.second["before"]["vec"];
                    std::vector<unsigned int> middle = rule.second["middle"]["vec"];
                    std::vector<unsigned int> after = rule.second["after"]["vec"];

                    r.loadRuleFromConfigInfo(rule.second["before"]["colour"],
                                            rule.second["middle"]["colour"],
                                            rule.second["after"]["colour"],
                                            before[0],//min
                                            before[1],//max, etc.
                                            middle[0],
                                            middle[1],
                                            after[0],
                                            after[1]);
                    //Neat method which is broken due to config system
                    /*r.loadRuleFromConfigInfo(rule.second["before"]["colour"],
                                            rule.second["middle"]["colour"],
                                            rule.second["after"]["colour"],
                                            static_cast<uint_32>(rule.second["before"]["vec"][0]),
                                            static_cast<uint_32>(rule.second["before"]["vec"][1]),
                                            static_cast<uint_32>(rule.second["middle"]["vec"][0]),
                                            static_cast<uint_32>(rule.second["middle"]["vec"][1]),
                                            static_cast<uint_32>(rule.second["after"]["vec"][0]),
                                            static_cast<uint_32>(rule.second["after"]["vec"][1]));
                                            unint_32(rule.second["after"]["vec"][1]));*/
                    segmentFilter.addTransitionRule(r);
                    //std::cout<< "Finished Config Loading successfully."<<std::endl;
                }
            });

            on<Trigger<Image>, With<LookUpTable, Raw<Image>, Raw<LookUpTable>>, Options<Single>>([this](const Image& image, const LookUpTable& lut, const std::shared_ptr<const Image>& imagePtr, const std::shared_ptr<const LookUpTable> lutPtr) { // TODO: fix!
                /*std::vector<arma::vec2> green_horizon_points = */
                //std::cout << "Image size = "<< image.width() << "x" << image.height() <<std::endl;
                //std::cout << "LUTClassifier::on<Trigger<Image>> calculateGreenHorizon" << std::endl;
                greenHorizon.calculateGreenHorizon(image, lut);

                //std::cout << "LUTClassifier::on<Trigger<Image>> generateScanLines" << std::endl;
                std::vector<int> generatedScanLines;
                SegmentedRegion horizontalClassifiedSegments, verticalClassifiedSegments;

                scanLines.generateScanLines(image, greenHorizon, &generatedScanLines);

                //std::cout << "LUTClassifier::on<Trigger<Image>> classifyHorizontalScanLines" << std::endl;
                scanLines.classifyHorizontalScanLines(image, generatedScanLines, lut, &horizontalClassifiedSegments);

                //std::cout << "LUTClassifier::on<Trigger<Image>> classifyVerticalScanLines" << std::endl;
                scanLines.classifyVerticalScanLines(image, greenHorizon, lut, &verticalClassifiedSegments);

                //std::cout << "LUTClassifier::on<Trigger<Image>> classifyImage" << std::endl;
                std::unique_ptr<ClassifiedImage> classifiedImage = segmentFilter.classifyImage(horizontalClassifiedSegments, verticalClassifiedSegments);
                classifiedImage->image = imagePtr;
                classifiedImage->LUT = lutPtr;
                classifiedImage->greenHorizonInterpolatedPoints = greenHorizon.getInterpolatedPoints();

                //std::cout << "LUTClassifier::on<Trigger<Image>> emit(std::move(classified_image));" << std::endl;
                emit(std::move(classifiedImage));
            });

            // on<Trigger<Image>>([this](const Image& image) {

            //  //NUClear::log("Waiting 100 milliseconds...");

            //  system_clock::time_point start = system_clock::now();

            //  while (std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now().time_since_epoch()) -
            //          std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch())  < std::chrono::milliseconds(3)){}

            // });
        }

    }  // vision
}  // modules
