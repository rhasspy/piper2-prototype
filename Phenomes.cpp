//This file is for inputting your custom phenome and voice testing phrases
//Ten tests, each generated into separate files with one combined file as well

#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>  // For directory handling
#include <piper2.h>

int main() {
    // Ensure the "testing" subdirectory exists
    const std::string output_dir = "testing";
    std::filesystem::create_directory(output_dir);  // Creates if not exists, does nothing if it does

    // The combined output file
    const std::string combined_filename = output_dir + "/Phonemes_Testing.raw";

    // Open the combined file early so we can append to it as we generate each part
    std::ofstream combined_stream(combined_filename, std::ios::binary);
    if (!combined_stream) {
        std::cerr << "Failed to open combined output file: " << combined_filename << std::endl;
        return 1;
    }

    // 20 string variables (10 pairs)
    std::string filename1 = "Short_Vowels_Simple_Consonants_1";
    std::string text1 = R"(Short vowels and simple consonants, part one.
        A black cat sat on the mat.A black cat sat on the mat.A black cat sat on the mat.The red hen met the wet pet.The red hen met the wet pet.The red hen met the wet pet.Six big pigs sit in the pit.Six big pigs sit in the pit.Six big pigs sit in the pit.
        )";

    std::string filename2 = "Short_Vowels_Simple_Consonants_2";
    std::string text2 = R"(Short vowels and simple consonants, part two.
Hot pots got dropped on the spot. Hot pots got dropped on the spot. Hot pots got dropped on the spot. Look, the book took the hook. Look, the book took the hook. Look, the book took the hook. But the cup must run up. But the cup must run up. But the cup must run up.
)";

    std::string filename3 = "Long_Vowels_Diphthongs_1";
    std::string text3 = R"(Long vowels and diphthongs, part one.
The day may stay gray. The day may stay gray. The day may stay gray. Pay the boy to play with the toy. Pay the boy to play with the toy. Pay the boy to play with the toy. Now the cow howls loud. Now the cow howls loud. Now the cow howls loud.
)";

    std::string filename4 = "Long_Vowels_Diphthongs_2";
    std::string text4 = R"(Long vowels and diphthongs, part two.
Buy high pie by the sky. Buy high pie by the sky. Buy high pie by the sky. Go slow, no, go home. Go slow, no, go home. Go slow, no, go home. Few new views grew blue. Few new views grew blue. Few new views grew blue.
)";

    std::string filename5 = "Voiced_Voiceless_Consonant_pairs_1";
    std::string text5 = R"(Voiced/voiceless consonant pairs, part one.
The bat pat dad had bad pad. The bat pat dad had bad pad. The bat pat dad had bad pad. Get the pet, let the vet set. Get the pet, let the vet set. Get the pet, let the vet set.
)";

    std::string filename6 = "Voiced_Voiceless_Consonant_pairs_2";
    std::string text6 = R"(Voiced/voiceless consonant pairs, part two.
Van fans ran to the zoo. Van fans ran to the zoo. Van fans ran to the zoo. Sue the judge in church. Sue the judge in church. Sue the judge in church.
)";

    std::string filename7 = "Nasals_Liquids_Fricatives_1";
    std::string text7 = R"(Nasals, liquids, and fricatives. part one.
The man ran and sang a long song. The man ran and sang a long song. The man ran and sang a long song. Yellow bells ring in the red light. Yellow bells ring in the red light. Yellow bells ring in the red light.
)";

    std::string filename8 = "Nasals_Liquids_Fricatives_2";
    std::string text8 = R"(Nasals, liquids, and fricatives, part two.
Right, the bright night light might fight. Right, the bright night light might fight. Right, the bright night light might fight. Think this thin myth with the thick path. Think this thin myth with the thick path. Think this thin myth with the thick path.
)";

    std::string filename9 = "Difficult_Clusters_Rare_Sounds_1";
    std::string text9 = R"(Difficult clusters and rare sounds, part one.
Strength helps sixth graders splash through thrash. Strength helps sixth graders splash through thrash. Strength helps sixth graders splash through thrash.
)";

    std::string filename10 = "Difficult_Clusters_Rare_Sounds_2";
    std::string text10 = R"(Difficult clusters and rare sounds. part two.
The squid gnaws on a gnome's knife wrist. The squid gnaws on a gnome's knife wrist. The squid gnaws on a gnome's knife wrist. Queue for the rhythm in the queue. Queue for the rhythm in the queue. Queue for the rhythm in the queue.
)";

    // Array of pairs
    std::pair<std::string, std::string> pairs[10] = {
        {filename1, text1},
        {filename2, text2},
        {filename3, text3},
        {filename4, text4},
        {filename5, text5},
        {filename6, text6},
        {filename7, text7},
        {filename8, text8},
        {filename9, text9},
        {filename10, text10}
    };

    // Create synthesizer once
    piper2_synthesizer* synth = piper2_create_phonemizer_stress(
        "en_US",
        "local/en_US-hfc_female-medium.onnx",
        NULL,
        "models/en_US-phonemizer.onnx",
        NULL,
        "models/en_US-stress.onnx");

    if (!synth) {
        std::cerr << "Failed to create synthesizer!" << std::endl;
        return 1;
    }

    piper2_synthesize_options options = piper2_default_synthesize_options(synth);

    // Generate each file
    for (int i = 0; i < 10; ++i) {
        std::string base_name = pairs[i].first;
        const std::string& input_text = pairs[i].second;

        // Automatically add ".raw" if not already present
        if (base_name.size() < 4 || base_name.substr(base_name.size() - 4) != ".raw") {
            base_name += ".raw";
        }

        std::string full_path = output_dir + "/" + base_name;

        std::cout << "Generating: " << full_path << std::endl;

        std::ofstream audio_stream(full_path, std::ios::binary);
        if (!audio_stream) {
            std::cerr << "Failed to open individual file: " << full_path << std::endl;
            continue;
        }

        piper2_synthesize_start(synth, input_text.c_str(), &options);

        piper2_audio_chunk chunk;
        while (piper2_synthesize_next(synth, &chunk) != PIPER2_DONE) {
            // Write to individual file
            audio_stream.write(reinterpret_cast<const char*>(chunk.samples),
                chunk.num_samples * sizeof(float));

            // Also write to combined file
            combined_stream.write(reinterpret_cast<const char*>(chunk.samples),
                chunk.num_samples * sizeof(float));

            // Debug output (optional, can comment out later)
            std::cout << "Text: " << chunk.chars << std::endl;
            std::cout << "Phonemes: " << chunk.phonemes << std::endl;
            std::cout << "Phoneme ids:";
            for (std::size_t j = 0; j < chunk.num_phoneme_ids; ++j) {
                std::cout << " " << chunk.phoneme_ids[j];
            }
            std::cout << std::endl << std::endl;
        }

        audio_stream.close();
        std::cout << "Finished: " << full_path << std::endl << std::endl;
    }

    // Cleanup
    combined_stream.close();
    piper2_free(synth);

    std::cout << "All done! 10 individual files saved in '" << output_dir << "/'" << std::endl;
    std::cout << "Combined file saved as: " << combined_filename << std::endl;

    return 0;
}