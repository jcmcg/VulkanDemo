#pragma once

typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long long ulong;

typedef enum {
  IE_OK,
  IE_IMAGE_FILE_OPEN,
  IE_IMAGE_FILE_READ,
  IE_IMAGE_SIGNATURE,
  IE_IMAGE_FORMAT,
  IE_IMAGE_CRC,
  IE_IMAGE_UNZIP
} IMAGE_ERROR;
const char *IMAGE_ERRORS[];

typedef enum {
  PS_UNKNOWN,
  PS_GREY,    // 8 bit / greyscale
  PS_IRGB,    // Indexed RGB
  PS_RGB,     // True RGB
  PS_RGBA,    // True RGB with Alpha
  PS_BGR,     // True BGR
  PS_BGRA     // True BGR with Alpha
} PIXEL_SCHEME;

typedef struct {
  uint width;
  uint height;
  PIXEL_SCHEME pixel_scheme;
  byte bpp;
  uint byte_width;
  byte *data;
  ulong data_length;
} image_t;

/* Portable Network Graphics */

#ifndef FOURCC
#define FOURCC(b0,b1,b2,b3) ((uint)(byte)(b0)      | \
                            ((uint)(byte)(b1)<< 8) | \
                            ((uint)(byte)(b2)<<16) | \
                            ((uint)(byte)(b3)<<24))
#endif
#define REVERSE32(q) ((((q)>>24)&0xff)+(((q)>>8)&0xff00)+(((q)&0xff00)<<8)+(((q)&0xff)<<24))

// PNG signatures
#define FOURCC_PNG1 FOURCC('\211', 'P', 'N', 'G')
#define FOURCC_PNG2 FOURCC('\r', '\n', '\032', '\n')
#define FOURCC_IHDR FOURCC('I', 'H', 'D', 'R')
#define FOURCC_pHYs FOURCC('p', 'H', 'Y', 's')
#define FOURCC_cHRM FOURCC('c', 'H', 'R', 'M')
#define FOURCC_gAMA FOURCC('g', 'A', 'M', 'A')
#define FOURCC_PLTE FOURCC('P', 'L', 'T', 'E')
#define FOURCC_tRNS FOURCC('t', 'R', 'N', 'S')
#define FOURCC_tEXt FOURCC('t', 'E', 'X', 't')
#define FOURCC_iTXt FOURCC('i', 'T', 'X', 't')
#define FOURCC_sRGB FOURCC('s', 'R', 'G', 'B')
#define FOURCC_iCCP FOURCC('i', 'C', 'C', 'P')
#define FOURCC_tIME FOURCC('t', 'I', 'M', 'E')
#define FOURCC_IDAT FOURCC('I', 'D', 'A', 'T')
#define FOURCC_IEND FOURCC('I', 'E', 'N', 'D')

#define MAX_IDAT_CHUNK_SIZE 65536

#pragma pack(push, 1)

// PNG IHDR chunk - 13 bytes
typedef struct {
  uint width;
  uint height;
  byte bit_depth;
  byte colour_type;
  byte compression_method;
  byte filter_method;
  byte interlace_method;
} PNGIHDRCHUNK;

// PNG pHYs chunk - 9 bytes
typedef struct {
  uint ppu_x;
  uint ppu_y;
  byte unit_specifier;
} PNGpHYsCHUNK;

// PNG cHRM chunk - 32 bytes
typedef struct {
  uint white_x;
  uint white_y;
  uint red_x;
  uint red_y;
  uint green_x;
  uint green_y;
  uint blue_x;
  uint blue_y;
} PNGcHRMCHUNK;

// PNG info header - 2123 bytes
typedef struct {
  PNGIHDRCHUNK IHDR;
  PNGpHYsCHUNK pHYs;
  PNGcHRMCHUNK cHRM;
  uint gamma;
  byte rendering_intent;
  byte *palette;
  ushort n_pal_entries;
  ushort r_transparency;
  ushort g_transparency;
  ushort b_transparency;
  byte *alpha_palette;
  long PNG_data_offsets[256];
  uint PNG_data_lengths[256];
  uint PNG_data_CRCs[256];
} PNGINFO;

#pragma pack(pop)

IMAGE_ERROR load_png(const char *, image_t *);
void destroy_image(image_t *);
