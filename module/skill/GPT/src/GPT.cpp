#include "GPT.hpp"

#include "extension/Configuration.hpp"

#include "message/skill/GPT.hpp"
#include "message/skill/Say.hpp"

#include "utility/openai/openai.hpp"

namespace module::skill {

    using extension::Configuration;
    using message::skill::GPTAudioRequest;
    using message::skill::GPTChatRequest;
    using message::skill::Say;

    GPT::GPT(std::unique_ptr<NUClear::Environment> environment) : BehaviourReactor(std::move(environment)) {

        on<Configuration>("GPT.yaml").then([this](const Configuration& config) {
            // Use configuration here from file GPT.yaml
            this->log_level    = config["log_level"].as<NUClear::LogLevel>();
            cfg.openai_api_key = config["openai_api_key"].as<std::string>();
        });

        on<Startup>().then([this] {
            // Configure the OpenAI API key
            utility::openai::start(cfg.openai_api_key);
        });

        on<Provide<GPTChatRequest>>().then([this](const GPTChatRequest& gpt_request, const RunInfo& info) {
            if (info.run_reason == RunInfo::NEW_TASK) {
                // Send request to OpenAI API
                nlohmann::json request = {
                    {"model", "gpt-3.5-turbo"},
                    {"messages", nlohmann::json::array({{{"role", "user"}, {"content", gpt_request.text}}})},
                    {"max_tokens", 100},
                    {"temperature", 0}};
                auto chat = utility::openai::chat().create(request);

                std::string response = chat["choices"][0]["message"]["content"].get<std::string>();
                log<NUClear::DEBUG>("Response: ", response);

                if (gpt_request.speak_response) {
                    emit<Task>(std::make_unique<Say>(response));
                }
            }
        });

        on<Provide<GPTAudioRequest>>().then([this](const GPTAudioRequest& gpt_request, const RunInfo& info) {
            if (info.run_reason == RunInfo::NEW_TASK) {
                // Record audio for requested time
                std::string cmd = "arecord -d " + std::to_string(gpt_request.record_time) + " -f cd -t wav audio.wav";

                // Execute the command to record audio and convert it to MP3
                log<NUClear::INFO>("Recording audio...");
                system(cmd.c_str());
                log<NUClear::INFO>("Finished recording audio.");
                // Convert audio to mp3
                system("lame audio.wav audio.mp3");
                log<NUClear::INFO>("Converted audio to mp3.");

                // Send request to OpenAI API
                auto audio = utility::openai::audio().transcribe(R"(
                    {
                    "file": "audio.mp3",
                    "model": "whisper-1"
                    }
                )"_json);

                std::string transcription = audio["text"].get<std::string>();
                log<NUClear::DEBUG>("Transcription: ", transcription);

                if (gpt_request.send_to_chat) {
                    emit<Task>(std::make_unique<GPTChatRequest>(transcription, gpt_request.speak_response));
                }
            }
        });
    }

}  // namespace module::skill
