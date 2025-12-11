#ifndef PIPER2_IMPL_H_
#define PIPER2_IMPL_H_

#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <stdint.h>
#include <string>
#include <vector>

#include <json.hpp>

#include <onnxruntime_cxx_api.h>

#include <unicode/brkiter.h>
#include <unicode/normalizer2.h>
#include <unicode/numfmt.h>
#include <unicode/rbnf.h>
#include <unicode/regex.h>
#include <unicode/schriter.h>
#include <unicode/translit.h>
#include <unicode/ubrk.h>
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/ucnv.h>
#include <unicode/udata.h>
#include <unicode/unistr.h>

typedef char32_t Phoneme;
typedef int64_t PhonemeId;
typedef int64_t SpeakerId;
typedef int64_t CharId;
typedef std::map<Phoneme, std::vector<PhonemeId>> PhonemeIdsMap; // multiple ids
typedef std::map<icu::UnicodeString, PhonemeId> PhonemeIdMap;
typedef std::map<PhonemeId, icu::UnicodeString> IdPhonemeMap;
typedef std::map<CharId, icu::UnicodeString> IdCharMap;
typedef std::map<icu::UnicodeString, CharId> CharIdMap;
typedef std::map<icu::UnicodeString, icu::UnicodeString> CharMap;
typedef std::map<icu::UnicodeString, icu::UnicodeString> PhonemeMap;

const PhonemeId ID_PAD = 0; // interleaved
const PhonemeId ID_BOS = 1; // beginning of sentence
const PhonemeId ID_EOS = 2; // end of sentence

const Phoneme PHONEME_PAD = U'_';
const Phoneme PHONEME_BOS = U'^';
const Phoneme PHONEME_EOS = U'$';
const Phoneme PHONEME_SEPARATOR = 0;

const float DEFAULT_LENGTH_SCALE = 1.0f;
const float DEFAULT_NOISE_SCALE = 0.667f;
const float DEFAULT_NOISE_W_SCALE = 0.8f;

// onnx
Ort::Env ort_env{ORT_LOGGING_LEVEL_WARNING, "piper2"};

struct piper2_synthesizer {
    // From voice config
    int sample_rate;
    int num_speakers;
    PhonemeIdsMap voice_phoneme_id_map;

    // Default synthesis settings for the voice
    float synth_length_scale = DEFAULT_LENGTH_SCALE;
    float synth_noise_scale = DEFAULT_NOISE_SCALE;
    float synth_noise_w_scale = DEFAULT_NOISE_W_SCALE;

    // phonemizer and stress configs
    CharIdMap phonemizer_char_id_map;
    IdCharMap phonemizer_id_char_map;
    PhonemeId phonemizer_phoneme_blank_id;
    PhonemeIdMap phonemizer_phoneme_id_map;
    IdPhonemeMap phonemizer_id_phoneme_map;
    CharMap phonemizer_char_map;
    PhonemeMap phonemizer_phoneme_map;
    icu::UnicodeString phonemizer_stress_char;

    // onnx
    std::unique_ptr<Ort::Session> voice_session;
    std::unique_ptr<Ort::Session> phonemizer_session;
    std::unique_ptr<Ort::Session> stress_session;
    Ort::AllocatorWithDefaultOptions session_allocator;
    Ort::SessionOptions session_options;
    Ort::Env session_env;

    // synthesize state
    std::queue<std::vector<CharId>> char_id_queue;
    std::vector<float> chunk_samples;
    std::string chunk_chars;
    std::string chunk_phonemes;
    std::vector<int> chunk_phoneme_ids;
    float length_scale = DEFAULT_LENGTH_SCALE;
    float noise_scale = DEFAULT_NOISE_SCALE;
    float noise_w_scale = DEFAULT_NOISE_W_SCALE;
    SpeakerId speaker_id = 0;

    // ICU
    icu::Locale locale;
    UErrorCode status = U_ZERO_ERROR;

    // Unicode normalization
    const icu::Normalizer2 *normalizer_nfd;
    std::unique_ptr<icu::Transliterator> transliterator;

    // Engine for transforming numbers into words
    std::unique_ptr<icu::RuleBasedNumberFormat> rbnf;
    std::optional<icu::UnicodeString> rbnf_year_rule;

    std::unique_ptr<icu::NumberFormat> num_format;
    std::unique_ptr<icu::RegexPattern> pattern_whitespace;
};

std::optional<char32_t> get_codepoint(const std::string &s) {
    auto us = icu::UnicodeString::fromUTF8(s);
    if (us.isEmpty()) {
        return std::nullopt;
    }
    UChar32 cp = us.char32At(0);
    return static_cast<char32_t>(cp);
}

#endif // PIPER2_IMPL_H_
