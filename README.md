# Piper 2

**EARLY ACCESS!**

This is an early prototype for Piper 2, a version of [Piper](https://github.com/OHF-Voice/piper1-gpl/) with an Apache license instead of GPL.

## Dependencies

* libicu
* onnxruntime (tested with 1.23.2)

## Building

Uses cmake and automatically downloads onnxunrtime.

## Testing

See `example.cpp` for how to use `libpiper2`.

## Phonemizer

Instead of using [espeak-ng](https://github.com/espeak-ng/espeak-ng) like Piper 1, pre-trained phonemizer and stress models for U.S. English is used. Both models are bidirectional LSTMs, and trained on the same IPA phoneme set as Piper 1.

Training code will be made available in the future.

## Voices

The existing [U.S. English Piper voices](https://huggingface.co/rhasspy/piper-voices/tree/main/en/en_US) should work with Piper 2.
