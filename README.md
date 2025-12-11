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

You must download a voice first:

``` sh
mkdir -p local
wget -O local/en_US-hfc_female-medium.onnx 'https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/hfc_female/medium/en_US-hfc_female-medium.onnx?download=true'
wget -O local/en_US-hfc_female-medium.onnx.json 'https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/hfc_female/medium/en_US-hfc_female-medium.onnx.json?download=true'
```


After building `libpiper2` with cmake, you can build and run the example with:

``` sh
g++ -Ilibpiper2/include -Lbuild -o example example.cpp -lpiper2 -licuuc -licui18n
LD_LIBRARY_PATH="${PWD}/lib:${PWD}/build:${LD_LIBRARY_PATH}" ./example
```

The output file `test.raw` will contain 16-bit mono float samples at 22050Hz. You can play this with:

``` sh
aplay -r 22050 -c 1 -f FLOAT -t raw test.raw
```


## Phonemizer

Instead of using [espeak-ng](https://github.com/espeak-ng/espeak-ng) like Piper 1, pre-trained phonemizer and stress models for U.S. English is used. Both models are bidirectional LSTMs, and trained on the same IPA phoneme set as Piper 1.

Training code will be made available in the future.

Expect some pronunciations to be incorrect!

## Voices

The existing [U.S. English Piper voices](https://huggingface.co/rhasspy/piper-voices/tree/main/en/en_US) should work with Piper 2.
