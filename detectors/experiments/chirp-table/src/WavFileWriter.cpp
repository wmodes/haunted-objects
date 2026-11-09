/**
 * @file WavFileWriter.cpp
 * @brief Implementation of WavFileWriter. See WavFileWriter.h.
 */
#include "WavFileWriter.h"

#include <cstdio>

namespace {

// Fixed properties of the format this writer produces (not tunable settings).
constexpr std::uint16_t kPcmFormatTag = 1;
constexpr std::uint16_t kChannelCount = 1;
constexpr std::uint16_t kBitsPerSample = 16;
constexpr std::uint32_t kFormatChunkSizeBytes = 16;
constexpr std::uint32_t kHeaderBytesAfterRiffSize = 36;  // "WAVE" + "fmt " chunk + "data" header

/** Writes `value` as little-endian bytes, as the WAV format requires, whatever the host order. */
template <typename Integer>
bool writeLittleEndian(std::FILE* file, Integer value) {
  for (std::size_t i = 0; i < sizeof(Integer); ++i) {
    const unsigned char byte = static_cast<unsigned char>((value >> (8 * i)) & 0xFF);
    if (std::fputc(byte, file) == EOF) {
      return false;
    }
  }
  return true;
}

bool writeText(std::FILE* file, const char* text) {
  return std::fputs(text, file) != EOF;
}

}  // namespace

bool WavFileWriter::write(const char* path, const std::vector<std::int16_t>& samples,
                          unsigned sampleRateHertz) {
  std::FILE* file = std::fopen(path, "wb");
  if (file == nullptr) {
    return false;
  }

  const std::uint32_t dataBytes = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));
  const std::uint16_t blockAlignBytes = kChannelCount * (kBitsPerSample / 8);
  const std::uint32_t byteRate = sampleRateHertz * blockAlignBytes;

  bool ok = writeText(file, "RIFF") && writeLittleEndian(file, kHeaderBytesAfterRiffSize + dataBytes) &&
            writeText(file, "WAVE") && writeText(file, "fmt ") &&
            writeLittleEndian(file, kFormatChunkSizeBytes) && writeLittleEndian(file, kPcmFormatTag) &&
            writeLittleEndian(file, kChannelCount) &&
            writeLittleEndian(file, static_cast<std::uint32_t>(sampleRateHertz)) &&
            writeLittleEndian(file, byteRate) && writeLittleEndian(file, blockAlignBytes) &&
            writeLittleEndian(file, kBitsPerSample) && writeText(file, "data") &&
            writeLittleEndian(file, dataBytes);

  for (std::size_t i = 0; ok && i < samples.size(); ++i) {
    ok = writeLittleEndian(file, static_cast<std::uint16_t>(samples[i]));
  }

  // Closing can fail too (a full disk shows up here), so it counts toward success.
  ok = (std::fclose(file) == 0) && ok;
  return ok;
}
