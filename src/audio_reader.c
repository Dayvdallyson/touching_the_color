#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
  char chunkID[4];
  uint32_t chunkSize;
  char format[4];

  char subchunk1ID[4];
  uint32_t subchunk1Size;
  uint16_t audioFormat;
  uint16_t numChannels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;

  char subchunk2ID[4];
  uint32_t subchunk2Size;
} WavHeader;
#pragma pack(pop)

int main() {
  FILE *file = fopen("tue_cogulandia.wav", "rb");
  if (!file) {
    perror("Error opening file");
    return 1;
  }
  WavHeader header;

  if (fread(&header, sizeof(WavHeader), 1, file) != 1) {
    printf("Error reading header\n");
    fclose(file);
    return 1;
  }

  if (header.chunkID[0] != 'R' || header.chunkID[1] != 'I' ||
      header.chunkID[2] != 'F' || header.chunkID[3] != 'F') {
    printf("Error: Not a valid RIFF file\n");
    fclose(file);
    return 1;
  }

  printf("Channels:       %d\n", header.numChannels);
  printf("Sample Rate:    %d Hz\n", header.sampleRate);
  printf("Bits per Sample:%d-bit\n", header.bitsPerSample);
  printf("Data Size:      %u bytes\n", header.subchunk2Size);

  uint8_t *audioBuffer = (uint8_t *)malloc(header.subchunk2Size);
  if (audioBuffer == NULL) {
    printf("Memory allocation failed\n");
    fclose(file);
    return 1;
  }

  size_t bytesRead = fread(audioBuffer, 1, header.subchunk2Size, file);

  if (bytesRead != header.subchunk2Size) {
    if (feof(file)) {
      fprintf(stderr, "Unexpected end of file while reading audio data\n");
    } else if (ferror(file)) {
      perror("Error reading audio data");
    }
    free(audioBuffer);
    fclose(file);
    return 1;
  }

  int bytesPerSample = header.bitsPerSample / 8;
  uint32_t totalSamples = header.subchunk2Size / bytesPerSample;
  uint32_t totalFrames = totalSamples / header.numChannels;

  printf("Total frames:   %u\n", totalFrames);

  uint32_t framesToPrint = totalFrames < 10 ? totalFrames : 10;

  for (uint32_t frame = 0; frame < framesToPrint; frame++) {
    printf("Frame %u ", frame);

    for (uint16_t ch = 0; ch < header.numChannels; ch++) {
      uint32_t idx = frame * header.numChannels + ch;
      int32_t value = 0;

      switch (header.bitsPerSample) {
      case 8: {
        value = (int32_t)audioBuffer[idx] - 128;
        break;
      }

      case 16: {
        int16_t s;
        memcpy(&s, audioBuffer + idx * 2, 2);
        value = s;
        break;
      }

      case 24: {
        uint8_t *p = audioBuffer + idx * 3;
        int32_t s = p[0] | (p[1] << 8) | (p[2] << 16);
        if (s & 0x800000)
          s |= ~0xFFFFFF;
        value = s;
        break;
      }

      case 32: {
        int32_t s;
        memcpy(&s, audioBuffer + idx * 4, 4);
        value = s;
        break;
      }
      }
      printf("%d ", value);
    }
    printf("\n");
  }

  free(audioBuffer);
  fclose(file);
  return 0;
}
