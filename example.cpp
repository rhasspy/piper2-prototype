#include <fstream>
#include <iostream>

#include <piper2.h>

int main() {
    piper2_synthesizer *synth = piper2_create_phonemizer_stress(
        /* locale */ "en_US",
        /* voice model */ "local/en_US-lessac-medium.onnx",
        /* voice config */ NULL,
        /* phonemizer model */ "models/en_US-phonemizer.onnx",
        /* phonemizer config */ NULL,
        /* stress model */ "models/en_US-stress.onnx");

    std::ofstream audio_stream("test.raw", std::ios::binary);

    piper2_synthesize_options options =
        piper2_default_synthesize_options(synth);

    // For models with multiple speakers
    // options.speaker_id = 5;

    piper2_synthesize_start(synth, "This is a test: 1 2 3 4!",
                            &options);

    piper2_audio_chunk chunk;
    while (piper2_synthesize_next(synth, &chunk) != PIPER2_DONE) {
        audio_stream.write(reinterpret_cast<const char *>(chunk.samples),
                           chunk.num_samples * sizeof(float));

        std::cout << "Text: " << chunk.chars << std::endl;
        std::cout << "Phonemes: " << chunk.phonemes << std::endl;
        std::cout << "Phoneme ids:";
        for (std::size_t i = 0; i < chunk.num_phoneme_ids; ++i) {
            std::cout << " " << chunk.phoneme_ids[i];
        }
        std::cout << std::endl;

        std::cout << std::endl;
    }

    piper2_free(synth);
    synth = nullptr;

    return 0;
}
