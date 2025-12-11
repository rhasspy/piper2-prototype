#include "piper2.h"
#include "piper2_impl.hpp"

#include <iostream> // TODO

#include <array>
#include <fstream>
#include <limits>

using json = nlohmann::json;

piper2_synthesizer *piper2_create_phonemizer_stress(
    const char *locale, const char *voice_model_path,
    const char *voice_config_path, const char *phonemizer_model_path,
    const char *phonemizer_config_path, const char *stress_model_path) {

    if (!voice_model_path || !phonemizer_model_path || !stress_model_path) {
        return nullptr;
    }

    // Resolve config paths
    std::string voice_config_path_str;
    if (!voice_config_path) {
        std::string voice_model_path_str(voice_model_path);
        voice_config_path_str = voice_model_path_str + ".json";
    } else {
        voice_config_path_str = voice_config_path;
    }

    std::string phonemizer_config_path_str;
    if (!phonemizer_config_path) {
        std::string phonemizer_model_path_str(phonemizer_model_path);
        phonemizer_config_path_str = phonemizer_model_path_str + ".json";
    } else {
        phonemizer_config_path_str = phonemizer_config_path;
    }

    // Load/validate configs
    std::ifstream voice_config_stream(voice_config_path_str);
    auto voice_config = json::parse(voice_config_stream);

    std::ifstream phonemizer_config_stream(phonemizer_config_path_str);
    auto phonemizer_config = json::parse(phonemizer_config_stream);

    if (!voice_config.contains("audio") ||
        !voice_config.contains("phoneme_id_map") ||
        !phonemizer_config.contains("char_id_map") ||
        !phonemizer_config.contains("phoneme_blank_id") ||
        !phonemizer_config.contains("phoneme_id_map") ||
        !phonemizer_config.contains("stress_char")) {
        // Bad config
        return nullptr;
    }

    piper2_synthesizer *synth = new piper2_synthesizer();

    // ICU
    if (locale) {
        synth->locale = icu::Locale(locale);
    } else {
        // Current locale
        synth->locale = icu::Locale();
    }

    synth->normalizer_nfd = icu::Normalizer2::getNFDInstance(synth->status);

    auto transliterator = icu::Transliterator::createInstance(
        "NFD; [:Nonspacing Mark:] Remove; NFC", UTRANS_FORWARD, synth->status);

    synth->transliterator.reset(transliterator);
    synth->rbnf = std::make_unique<icu::RuleBasedNumberFormat>(
        icu::URBNF_SPELLOUT, synth->locale, synth->status);

    // Find names for other rules
    int32_t num_rule_sets = synth->rbnf->getNumberOfRuleSetNames();
    for (int32_t rule_idx = 0; rule_idx < num_rule_sets; rule_idx++) {
        auto rule_set_name = synth->rbnf->getRuleSetName(rule_idx);
        if (rule_set_name.endsWith("-year")) {
            synth->rbnf_year_rule = rule_set_name;
        }
    }

    synth->num_format.reset(
        icu::NumberFormat::createInstance(synth->locale, synth->status));

    // Load voice config
    {
        auto &audio_obj = voice_config["audio"];
        if (audio_obj.contains("sample_rate")) {
            // Sample rate of generated audio in hertz
            synth->sample_rate = audio_obj["sample_rate"].get<int>();
        }
    }

    // phoneme to [id] map
    // Maps phonemes to one or more phoneme ids (required).
    {
        auto &phoneme_id_map_value = voice_config["phoneme_id_map"];
        for (auto &from_phoneme_item : phoneme_id_map_value.items()) {
            std::string from_phoneme = from_phoneme_item.key();
            auto from_codepoint = get_codepoint(from_phoneme);
            if (!from_codepoint) {
                // No codepoint
                continue;
            }

            for (auto &to_id_value : from_phoneme_item.value()) {
                PhonemeId to_id = to_id_value.get<PhonemeId>();
                synth->voice_phoneme_id_map[*from_codepoint].push_back(to_id);
            }
        }
    }

    synth->num_speakers = voice_config["num_speakers"].get<SpeakerId>();

    if (voice_config.contains("inference")) {
        // Overrides default inference settings
        auto inference_value = voice_config["inference"];
        if (inference_value.contains("noise_scale")) {
            synth->synth_noise_scale =
                inference_value["noise_scale"].get<float>();
        }

        if (inference_value.contains("length_scale")) {
            synth->synth_length_scale =
                inference_value["length_scale"].get<float>();
        }

        if (inference_value.contains("noise_w")) {
            synth->synth_noise_w_scale =
                inference_value["noise_w"].get<float>();
        }
    }

    // Load phonemizer config
    {
        // char -> id
        auto &char_id_map_value = phonemizer_config["char_id_map"];
        for (auto &from_char_item : char_id_map_value.items()) {
            std::string from_char = from_char_item.key();
            auto from_char_unicode = icu::UnicodeString::fromUTF8(from_char);
            auto to_id = from_char_item.value().get<CharId>();

            synth->phonemizer_char_id_map[from_char_unicode] = to_id;

            // Also store id -> char
            synth->phonemizer_id_char_map[to_id] = from_char_unicode;
        }
    }

    synth->phonemizer_phoneme_blank_id =
        phonemizer_config["phoneme_blank_id"].get<PhonemeId>();

    {
        // phoneme -> id
        auto &phoneme_id_map_value = phonemizer_config["phoneme_id_map"];
        for (auto &from_phoneme_item : phoneme_id_map_value.items()) {
            std::string from_phoneme = from_phoneme_item.key();
            auto from_phoneme_unicode =
                icu::UnicodeString::fromUTF8(from_phoneme);
            auto to_id = from_phoneme_item.value().get<PhonemeId>();

            synth->phonemizer_phoneme_id_map[from_phoneme_unicode] = to_id;

            // Also store id -> phoneme
            synth->phonemizer_id_phoneme_map[to_id] = from_phoneme_unicode;
        }
    }

    if (phonemizer_config.contains("char_map")) {
        // char -> char
        auto &char_map_value = phonemizer_config["char_map"];
        for (auto &from_char_item : char_map_value.items()) {
            std::string from_char = from_char_item.key();
            auto from_char_unicode = icu::UnicodeString::fromUTF8(from_char);

            synth->phonemizer_char_map[from_char_unicode] =
                icu::UnicodeString::fromUTF8(
                    from_char_item.value().get<std::string>());
        }
    }

    if (phonemizer_config.contains("phoneme_map")) {
        // phoneme -> phoneme
        auto &phoneme_map_value = phonemizer_config["phoneme_map"];
        for (auto &from_phoneme_item : phoneme_map_value.items()) {
            std::string from_phoneme = from_phoneme_item.key();
            auto from_phoneme_unicode =
                icu::UnicodeString::fromUTF8(from_phoneme);

            synth->phonemizer_phoneme_map[from_phoneme_unicode] =
                icu::UnicodeString::fromUTF8(
                    from_phoneme_item.value().get<std::string>());
        }
    }

    synth->phonemizer_stress_char = icu::UnicodeString::fromUTF8(
        phonemizer_config["stress_char"].get<std::string>());

    // Load ONNX models
    synth->session_options.DisableCpuMemArena();
    synth->session_options.DisableMemPattern();
    synth->session_options.DisableProfiling();

    synth->voice_session = std::make_unique<Ort::Session>(
        Ort::Session(ort_env, voice_model_path, synth->session_options));

    synth->phonemizer_session = std::make_unique<Ort::Session>(
        Ort::Session(ort_env, phonemizer_model_path, synth->session_options));

    synth->stress_session = std::make_unique<Ort::Session>(
        Ort::Session(ort_env, stress_model_path, synth->session_options));

    return synth;
}

void piper2_free(struct piper2_synthesizer *synth) {
    if (!synth) {
        return;
    }

    delete synth;
}

piper2_synthesize_options
piper2_default_synthesize_options(piper2_synthesizer *synth) {
    piper2_synthesize_options options;
    options.speaker_id = 0;
    options.length_scale = DEFAULT_LENGTH_SCALE;
    options.noise_scale = DEFAULT_NOISE_SCALE;
    options.noise_w_scale = DEFAULT_NOISE_W_SCALE;

    if (synth) {
        options.length_scale = synth->synth_length_scale;
        options.noise_scale = synth->synth_noise_scale;
        options.noise_w_scale = synth->synth_noise_w_scale;
    }

    return options;
}

int piper2_synthesize_start(struct piper2_synthesizer *synth, const char *text,
                            const piper2_synthesize_options *options) {
    if (!synth) {
        return PIPER2_ERR_GENERIC;
    }

    // Clear state
    while (!synth->char_id_queue.empty()) {
        synth->char_id_queue.pop();
    }
    synth->chunk_samples.clear();

    std::unique_ptr<piper2_synthesize_options> default_options;
    if (!options) {
        default_options = std::make_unique<piper2_synthesize_options>(
            piper2_default_synthesize_options(synth));
        options = default_options.get();
    }

    synth->length_scale = options->length_scale;
    synth->noise_scale = options->noise_scale;
    synth->noise_w_scale = options->noise_w_scale;
    synth->speaker_id = options->speaker_id;

    // Normalize text (remove accents, NFC)
    auto text_unicode = icu::UnicodeString::fromUTF8(text).toLower();
    text_unicode.insert(0, " "); // phonemizer expects a leading space
    synth->transliterator->transliterate(text_unicode);

    // Split into sentences and map chars to ids
    auto sen_iter = std::unique_ptr<icu::BreakIterator>(
        icu::BreakIterator::createSentenceInstance(synth->locale,
                                                   synth->status));

    auto word_iter = std::unique_ptr<icu::BreakIterator>(
        icu::BreakIterator::createWordInstance(synth->locale, synth->status));

    auto char_iter = std::unique_ptr<icu::BreakIterator>(
        icu::BreakIterator::createCharacterInstance(synth->locale,
                                                    synth->status));

    sen_iter->setText(text_unicode);

    int sen_start = 0;
    int32_t sen_end = sen_iter->next();
    while (sen_end != icu::BreakIterator::DONE) {
        auto sen_text =
            icu::UnicodeString(text_unicode, sen_start, sen_end - sen_start);

        // numbers -> words
        std::vector<icu::UnicodeString> words;
        word_iter->setText(sen_text);

        int word_start = 0;
        int32_t word_end = word_iter->next();
        while (word_end != icu::BreakIterator::DONE) {
            auto word_text =
                icu::UnicodeString(sen_text, word_start, word_end - word_start);

            bool is_number = false;
            if (word_iter->getRuleStatus() == UBRK_WORD_NUMBER) {
                // Attempt to parse as a number
                icu::Formattable number_result;
                UErrorCode number_status = U_ZERO_ERROR;
                synth->num_format->parse(word_text, number_result,
                                         number_status);

                if (!U_FAILURE(number_status)) {
                    // Check if we need to override the default rule set
                    icu::UnicodeString number_unicode("");

                    // Format integer or floating point, either with the default
                    // or overriden rule set.
                    icu::FieldPosition pos = 0;
                    switch (number_result.getType()) {
                    case icu::Formattable::Type::kLong: {
                        auto long_number = number_result.getLong();
                        if (((long_number > 1000) || (long_number < 3000)) &&
                            synth->rbnf_year_rule) {
                            synth->rbnf->format(
                                long_number, *(synth->rbnf_year_rule),
                                number_unicode, pos, synth->status);

                        } else {
                            synth->rbnf->format(number_result.getLong(),
                                                number_unicode);
                        }
                        break;
                    }

                    case icu::Formattable::Type::kDouble: {
                        synth->rbnf->format(number_result.getDouble(),
                                            number_unicode);
                        break;
                    }

                    default: {
                        break;
                    }
                    }

                    if (!number_unicode.isEmpty()) {
                        is_number = true;
                        words.push_back(number_unicode);
                    }
                }
            }

            if (!is_number) {
                words.push_back(word_text);
            }

            // Next word
            word_start = word_end;
            word_end = word_iter->next();
        }

        // Split into characters (graphemes)
        std::vector<CharId> sen_char_ids;
        for (auto word_text : words) {
            char_iter->setText(word_text);
            int char_start = 0;
            int32_t char_end = char_iter->next();
            while (char_end != icu::BreakIterator::DONE) {
                auto char_text = icu::UnicodeString(word_text, char_start,
                                                    char_end - char_start);

                // Map chars
                auto char_map_iter = synth->phonemizer_char_map.find(char_text);
                if (char_map_iter != synth->phonemizer_char_map.end()) {
                    // Mapped char
                    char_text = char_map_iter->second;
                }

                auto char_id_map_iter =
                    synth->phonemizer_char_id_map.find(char_text);
                if (char_id_map_iter != synth->phonemizer_char_id_map.end()) {
                    sen_char_ids.push_back(char_id_map_iter->second);
                }

                // Next character
                char_start = char_end;
                char_end = char_iter->next();
            } // for each character

        } // for each word

        synth->char_id_queue.emplace(sen_char_ids);

        // Next sentence
        sen_start = sen_end;
        sen_end = sen_iter->next();
    } // for each sentence

    return PIPER2_OK;
}

int piper2_synthesize_next(struct piper2_synthesizer *synth,
                           struct piper2_audio_chunk *chunk) {
    if (!synth || !chunk) {
        return PIPER2_ERR_GENERIC;
    }

    // Clear data from previous call
    synth->chunk_samples.clear();
    synth->chunk_chars = "";
    synth->chunk_phonemes = "";
    synth->chunk_phoneme_ids.clear();

    chunk->sample_rate = synth->sample_rate;
    chunk->samples = nullptr;
    chunk->num_samples = 0;
    chunk->is_last = false;

    if (synth->char_id_queue.empty()) {
        // Empty final chunk
        chunk->is_last = true;
        return PIPER2_DONE;
    }

    // Process next list of phoneme ids
    auto next_char_ids = std::move(synth->char_id_queue.front());
    synth->char_id_queue.pop();

    icu::UnicodeString chunk_chars_unicode;
    for (auto char_id : next_char_ids) {
        chunk_chars_unicode.append(synth->phonemizer_id_char_map[char_id]);
    }
    chunk_chars_unicode.toUTF8String(synth->chunk_chars);

    auto memoryInfo = Ort::MemoryInfo::CreateCpu(
        OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

    // ---------
    // Phonemize
    // ---------
    std::vector<int64_t> phoneme_ids;

    {
        std::vector<Ort::Value> input_tensors;
        std::vector<int64_t> char_ids_shape{1, (int64_t)next_char_ids.size()};
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
            memoryInfo, next_char_ids.data(), next_char_ids.size(),
            char_ids_shape.data(), char_ids_shape.size()));

        std::array<const char *, 1> input_names = {"input_ids"};

        // Get all output names
        std::vector<std::string> output_names_strs =
            synth->phonemizer_session->GetOutputNames();

        std::vector<const char *> output_names;
        for (const auto &name : output_names_strs) {
            output_names.push_back(name.c_str());
        }

        // char ids -> phoneme ids
        auto output_tensors = synth->phonemizer_session->Run(
            Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(),
            input_tensors.size(), output_names.data(), output_names.size());

        if ((output_tensors.size() < 1) ||
            (!output_tensors.front().IsTensor())) {
            return PIPER2_ERR_GENERIC;
        }

        auto output_shape =
            output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
        auto num_logits = output_shape[output_shape.size() - 2];
        auto num_phoneme_ids = output_shape[output_shape.size() - 1];

        // logits (assuming batch size of 1)
        const float *output_data =
            output_tensors.front().GetTensorData<float>();

        std::optional<int64_t> prev_id;
        for (std::size_t logits_idx = 0; logits_idx < num_logits;
             ++logits_idx) {

            std::size_t data_offset = logits_idx * num_phoneme_ids;

            // skip softmax and use logits directly
            std::optional<float> best_logit;
            std::size_t best_phoneme_id = 0;
            for (std::size_t phoneme_id = 0; phoneme_id < num_phoneme_ids;
                 ++phoneme_id) {
                float logit = output_data[data_offset + phoneme_id];
                if (!best_logit || (logit > best_logit)) {
                    best_logit = logit;
                    best_phoneme_id = phoneme_id;
                }
            }

            // CTC decoding
            if (best_phoneme_id == synth->phonemizer_phoneme_blank_id) {
                prev_id.reset();
                continue;
            }

            if (prev_id && (*prev_id == best_phoneme_id)) {
                // CTC repeat
                continue;
            }

            phoneme_ids.push_back(best_phoneme_id);
            prev_id = best_phoneme_id;
        } // for each logit group

        // Clean up
        for (std::size_t i = 0; i < output_tensors.size(); i++) {
            Ort::detail::OrtRelease(output_tensors[i].release());
        }

        for (std::size_t i = 0; i < input_tensors.size(); i++) {
            Ort::detail::OrtRelease(input_tensors[i].release());
        }
    } // phonemize

    // ------
    // Stress
    // ------
    std::vector<icu::UnicodeString> phonemes;

    {
        std::vector<Ort::Value> input_tensors;
        std::vector<int64_t> phoneme_ids_shape{1, (int64_t)phoneme_ids.size()};
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
            memoryInfo, phoneme_ids.data(), phoneme_ids.size(),
            phoneme_ids_shape.data(), phoneme_ids_shape.size()));

        std::array<const char *, 1> input_names = {"phoneme_ids"};

        // Get all output names
        std::vector<std::string> output_names_strs =
            synth->phonemizer_session->GetOutputNames();

        std::vector<const char *> output_names;
        for (const auto &name : output_names_strs) {
            output_names.push_back(name.c_str());
        }

        // phoneme_ids -> stress probability
        auto output_tensors = synth->stress_session->Run(
            Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(),
            input_tensors.size(), output_names.data(), output_names.size());

        if ((output_tensors.size() < 1) ||
            (!output_tensors.front().IsTensor())) {
            return PIPER2_ERR_GENERIC;
        }

        auto output_shape =
            output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
        auto num_probabilities = output_shape[output_shape.size() - 1];

        if (num_probabilities == phoneme_ids.size()) {
            // probabilities (assuming batch size of 1)
            const float *output_data =
                output_tensors.front().GetTensorData<float>();

            for (std::size_t prob_idx = 0; prob_idx < num_probabilities;
                 ++prob_idx) {

                float prob = output_data[prob_idx];
                if (prob > 0.5) {
                    // Insert primary stress marker before vowel
                    phonemes.push_back(synth->phonemizer_stress_char);
                }

                auto phoneme_id = phoneme_ids[prob_idx];
                phonemes.push_back(
                    synth->phonemizer_id_phoneme_map[phoneme_id]);
            }
        }

        // Clean up
        for (std::size_t i = 0; i < output_tensors.size(); i++) {
            Ort::detail::OrtRelease(output_tensors[i].release());
        }

        for (std::size_t i = 0; i < input_tensors.size(); i++) {
            Ort::detail::OrtRelease(input_tensors[i].release());
        }
    } // stress

    icu::UnicodeString chunk_phonemes_unicode;
    for (auto phoneme : phonemes) {
        chunk_phonemes_unicode.append(phoneme);
    }
    chunk_phonemes_unicode.toUTF8String(synth->chunk_phonemes);

    // ---------
    // Synthesis
    // ---------

    {
        auto codepoint_iter = std::unique_ptr<icu::BreakIterator>(
            icu::BreakIterator::createCharacterInstance(synth->locale,
                                                        synth->status));

        // voice model expects NFD codepoints as phonemes
        std::vector<PhonemeId> syn_phoneme_ids{ID_BOS, ID_PAD};
        for (auto phoneme : phonemes) {
            phoneme = synth->normalizer_nfd->normalize(phoneme, synth->status);
            codepoint_iter->setText(phoneme);

            int codepoint_start = 0;
            int32_t codepoint_end = codepoint_iter->next();
            while (codepoint_end != icu::BreakIterator::DONE) {
                auto codepoint_text = icu::UnicodeString(
                    phoneme, codepoint_start, codepoint_end - codepoint_start);

                auto phoneme_id_iter = synth->voice_phoneme_id_map.find(
                    codepoint_text.char32At(0));
                if (phoneme_id_iter != synth->voice_phoneme_id_map.end()) {
                    for (auto phoneme_id : phoneme_id_iter->second) {
                        syn_phoneme_ids.push_back(phoneme_id);
                        syn_phoneme_ids.push_back(ID_PAD);
                    }
                }

                // Next codepoint
                codepoint_start = codepoint_end;
                codepoint_end = codepoint_iter->next();
            } // for each codepoint
        }
        syn_phoneme_ids.push_back(ID_EOS);

        std::vector<int64_t> phoneme_id_lengths{
            (int64_t)syn_phoneme_ids.size()};
        std::vector<float> scales{synth->noise_scale, synth->length_scale,
                                  synth->noise_w_scale};
        std::vector<Ort::Value> input_tensors;
        std::vector<int64_t> phoneme_ids_shape{1,
                                               (int64_t)syn_phoneme_ids.size()};
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
            memoryInfo, syn_phoneme_ids.data(), syn_phoneme_ids.size(),
            phoneme_ids_shape.data(), phoneme_ids_shape.size()));

        std::vector<int64_t> phoneme_id_lengths_shape{
            (int64_t)phoneme_id_lengths.size()};
        input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
            memoryInfo, phoneme_id_lengths.data(), phoneme_id_lengths.size(),
            phoneme_id_lengths_shape.data(), phoneme_id_lengths_shape.size()));

        std::vector<int64_t> scales_shape{(int64_t)scales.size()};
        input_tensors.push_back(Ort::Value::CreateTensor<float>(
            memoryInfo, scales.data(), scales.size(), scales_shape.data(),
            scales_shape.size()));

        // Add speaker id.
        // NOTE: These must be kept outside the "if" below to avoid being
        // deallocated.
        std::vector<int64_t> speaker_id{(int64_t)synth->speaker_id};
        std::vector<int64_t> speaker_id_shape{(int64_t)speaker_id.size()};

        if (synth->num_speakers > 1) {
            input_tensors.push_back(Ort::Value::CreateTensor<int64_t>(
                memoryInfo, speaker_id.data(), speaker_id.size(),
                speaker_id_shape.data(), speaker_id_shape.size()));
        }

        // From export_onnx.py
        std::array<const char *, 4> input_names = {"input", "input_lengths",
                                                   "scales", "sid"};

        // Get all output names
        std::vector<std::string> output_names_strs =
            synth->voice_session->GetOutputNames();
        std::vector<const char *> output_names;
        for (const auto &name : output_names_strs) {
            output_names.push_back(name.c_str());
        }

        // Infer
        auto output_tensors = synth->voice_session->Run(
            Ort::RunOptions{nullptr}, input_names.data(), input_tensors.data(),
            input_tensors.size(), output_names.data(), output_names.size());

        if ((output_tensors.size() < 1) ||
            (!output_tensors.front().IsTensor())) {
            return PIPER2_ERR_GENERIC;
        }

        auto audio_shape =
            output_tensors.front().GetTensorTypeAndShapeInfo().GetShape();
        chunk->num_samples = audio_shape[audio_shape.size() - 1];

        const float *audio_tensor_data =
            output_tensors.front().GetTensorData<float>();
        synth->chunk_samples.resize(chunk->num_samples);
        std::copy(audio_tensor_data, audio_tensor_data + chunk->num_samples,
                  synth->chunk_samples.begin());
        chunk->samples = synth->chunk_samples.data();
        chunk->chars = synth->chunk_chars.c_str();
        chunk->phonemes = synth->chunk_phonemes.c_str();

        for (auto phoneme_id : syn_phoneme_ids) {
            synth->chunk_phoneme_ids.push_back(phoneme_id);
        }
        chunk->phoneme_ids = synth->chunk_phoneme_ids.data();
        chunk->num_phoneme_ids = synth->chunk_phoneme_ids.size();

        chunk->is_last = synth->char_id_queue.empty();

        // Clean up
        for (std::size_t i = 0; i < output_tensors.size(); i++) {
            Ort::detail::OrtRelease(output_tensors[i].release());
        }

        for (std::size_t i = 0; i < input_tensors.size(); i++) {
            Ort::detail::OrtRelease(input_tensors[i].release());
        }
    }

    return PIPER2_OK;
}
