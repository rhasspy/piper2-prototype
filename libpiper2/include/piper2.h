#ifndef PIPER2_H_
#define PIPER2_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PIPER2_OK 0
#define PIPER2_DONE 1
#define PIPER2_ERR_GENERIC -1

/**
 * \brief Text-to-speech synthesizer.
 */
typedef struct piper2_synthesizer piper2_synthesizer;

/**
 * \brief Chunk of synthesized audio samples.
 */
typedef struct piper2_audio_chunk {
  /**
   * \brief Raw samples returned from the voice model.
   */
  const float *samples;

  /**
   * \brief Number of samples in the audio chunk.
   */
  size_t num_samples;

  /**
   * \brief Sample rate in Hertz.
   */
  int sample_rate;

  /**
   * \brief True if this is the last audio chunk.
   */
  bool is_last;

  // TODO: For debugging only. Not the final API.
  const char *chars;
  const char *phonemes;
  const int* phoneme_ids;
  size_t num_phoneme_ids;

} piper2_audio_chunk;

/**
 * \brief Options for synthesis.
 *
 * \sa \ref piper2_default_synthesize_options
 */
typedef struct piper2_synthesize_options {
  /**
   * \brief Id of speaker to use (multi-speaker models only).
   *
   * Id 0 is the first speaker.
   */
  int speaker_id;

  /**
   * \brief How fast the text is spoken.
   *
   * A length scale of 0.5 means to speak twice as fast.
   * A length scale of 2.0 means to speak twice as slow.
   * The default is 1.0.
   */
  float length_scale;

  /**
   * \brief Controls how much noise is added during synthesis.
   *
   * The best value depends on the voice.
   * For single speaker models, a value of 0.667 is usually good.
   * For multi-speaker models, a value of 0.333 is usually good.
   */
  float noise_scale;

  /**
   * \brief Controls how much phonemes vary in length during synthesis.
   *
   * The best value depends on the voice.
   * For single speaker models, a value of 0.8 is usually good.
   * For multi-speaker models, a value of 0.333 is usually good.
   */
  float noise_w_scale;
} piper2_synthesize_options;

/**
 * \brief Create a Piper text-to-speech synthesizer with a phonemizer and stress
 * model.
 *
 * \param locale ICU locale to use (e.g., \c en_US) or \c NULL for current
 * locale.
 *
 * \param voice_model_path path to ONNX voice model file.
 *
 * \param voice_config_path path to JSON voice config file or NULL if it's the
 * model path + .json.
 *
 * \param phonemizer_model_path path to ONNX phonemizer model file.
 *
 * \param phonemizer_config_path path to JSON phonemizer config file or NULL if
 * it's the model path + .json.
 *
 * \param stress_model_path path to ONNX stress model file.
 *
 * \return a Piper text-to-speech synthesizer for the voice/phonemizer/stress
 * models.
 */
piper2_synthesizer *piper2_create_phonemizer_stress(
    const char *locale, const char *voice_model_path,
    const char *voice_config_path, const char *phonemizer_model_path,
    const char *phonemizer_config_path, const char *stress_model_path);

/**
 * \brief Free resources for Piper synthesizer.
 *
 * \param synth Piper synthesizer.
 */
void piper2_free(piper2_synthesizer *synth);

/**
 * \brief Get the default synthesis options for a Piper synthesizer.
 *
 * \param synth Piper synthesizer.
 *
 * \return synthesis options from voice config.
 */
piper2_synthesize_options
piper2_default_synthesize_options(piper2_synthesizer *synth);

/**
 * \brief Start text-to-speech synthesis.
 *
 * \param synth Piper synthesizer.
 *
 * \param text text to synthesize into audio.
 *
 * \param options synthesis options or NULL for defaults.
 *
 * \sa \ref piper2_synthesize_next
 *
 * \return PIPER2_OK or error code.
 */
int piper2_synthesize_start(piper2_synthesizer *synth, const char *text,
                            const piper2_synthesize_options *options);

/**
 * \brief Synthesize next chunk of audio.
 *
 * \param synth Piper synthesizer.
 *
 * \param chunk audio chunk to fill.
 *
 * piper2_synthesize_start must be called before this function.
 * Each call to piper2_synthesize_next will fill the audio chunk, invalidating
 * the memory of the previous chunk.
 * The final audio chunk will have is_last = true.
 * A return value of PIPER2_DONE indicates that synthesis is complete.
 *
 * \sa \ref piper2_synthesize_start
 *
 * \return PIPER2_DONE when complete, otherwise PIPER2_OK or error code.
 */
int piper2_synthesize_next(piper2_synthesizer *synth,
                           piper2_audio_chunk *chunk);

#ifdef __cplusplus
}
#endif

#endif // PIPER2_H_
