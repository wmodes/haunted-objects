/**
 * @file WavFileWriter.h
 * @brief Writes mono 16-bit PCM audio to a WAV file.
 */
#pragma once

#include <cstdint>
#include <vector>

class WavFileWriter {
 public:
  /**
   * Writes `samples` as a mono, 16-bit PCM WAV file.
   *
   * @param path Where to write the file; an existing file is overwritten.
   * @param samples Mono 16-bit samples.
   * @param sampleRateHertz Sample rate of `samples`.
   * @return True if the whole file was written; false if the file could not be opened or written.
   */
  static bool write(const char* path, const std::vector<std::int16_t>& samples,
                    unsigned sampleRateHertz);
};
