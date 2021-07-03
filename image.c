#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <zlib125/zlib.h>
#include "image.h"

const char *IMAGE_ERRORS[] = {
  "OK",
  "Could not open image file",
  "Could not read image file",
  "Invalid image signature",
  "Invalid file format",
  "CRC failed",
  "Unzip failed"
};

IMAGE_ERROR close(FILE *fp, IMAGE_ERROR ie) {
  fclose(fp);
  return ie;
}

IMAGE_ERROR load_png(const char *path, image_t *image) {
  FILE *fp;
  if (fopen_s(&fp, path, "rb"))
    return IE_IMAGE_FILE_OPEN;

  long length = -1;
  if (!fseek(fp, 0, SEEK_END)) {
    length = ftell(fp);
    rewind(fp);
  }
  if (length <= 0)
    return close(fp, IE_IMAGE_FILE_READ);

  unsigned int fourcc;
  if (!(fread(&fourcc, sizeof fourcc, 1, fp) == 1 &&
        fourcc == FOURCC_PNG1 &&
        fread(&fourcc, sizeof fourcc, 1, fp) == 1 &&
        fourcc == FOURCC_PNG2))
    return close(fp, IE_IMAGE_SIGNATURE);

  uint n_chunks,
       chunk_id = 0,
       chunk_length,
       chunk_crc;
  byte n_IDAT_chunks = 0,
       *chunk_data = NULL;

  PNGINFO png_info;
  memset(&png_info, 0, sizeof (PNGINFO));

  for (n_chunks = 0; chunk_id != FOURCC_IEND; n_chunks++) {
    // Read chunk length
    if (fread(&chunk_length, sizeof (uint), 1, fp) != 1)
      return close(fp, IE_IMAGE_FILE_READ);
    chunk_length = REVERSE32(chunk_length); // Numbers are big-endian
    // Read chunk ID
    if (fread(&chunk_id, sizeof (uint), 1, fp) != 1)
      return close(fp, IE_IMAGE_FORMAT);
    if (!(chunk_length || // 0th chunk cannot be empty
          n_chunks && chunk_id == FOURCC_IEND)) // Only IEND chunk may be empty
      return close(fp, IE_IMAGE_FORMAT);
    switch (chunk_id) {
      case FOURCC_IHDR:
        if (n_chunks || // IHDR can only be 0th chunk
            chunk_length != sizeof (PNGIHDRCHUNK) ||
            fread(chunk_data = (byte *)&png_info.IHDR, chunk_length, 1, fp) != 1 ||
            // Width and height must be > 0
            !(png_info.IHDR.width && png_info.IHDR.height) ||
            // Compression method must be 0
            png_info.IHDR.compression_method ||
            // Filter method must be 0
            png_info.IHDR.filter_method ||
            // Support only non-interlaced PNGs
            png_info.IHDR.interlace_method ||
            // Support only True Colour, Indexed Colour or True Colour with Alpha PNGs
            !(png_info.IHDR.colour_type == 2 || png_info.IHDR.colour_type == 3 || png_info.IHDR.colour_type == 6) ||
            // Support only 8 bit PNGs
            png_info.IHDR.bit_depth != 8)
          return close(fp, IE_IMAGE_FORMAT);
        break;
      case FOURCC_sRGB:
        if (chunk_length != 1 ||
            fread(chunk_data = (byte *)&png_info.rendering_intent, chunk_length, 1, fp) != 1)
          return close(fp, IE_IMAGE_FORMAT);
        break;
      case FOURCC_gAMA:
        if (chunk_length != sizeof (uint) ||
            fread(chunk_data = (byte *)&png_info.gamma, chunk_length, 1, fp) != 1)
          return close(fp, IE_IMAGE_FORMAT);
        break;
      case FOURCC_pHYs:
        if (chunk_length != sizeof (PNGpHYsCHUNK) ||
            fread(chunk_data = (byte *)&png_info.pHYs, chunk_length, 1, fp) != 1)
          return close(fp, IE_IMAGE_FORMAT);
        break;
      case FOURCC_cHRM:
        if (chunk_length != sizeof (PNGcHRMCHUNK) ||
            fread(&png_info.cHRM, chunk_length, 1, fp) != 1)
          return close(fp, IE_IMAGE_FORMAT);
        break;
      case FOURCC_PLTE:
        break;
      case FOURCC_tRNS:
        break;
      case FOURCC_tEXt:
      case FOURCC_iTXt:
      case FOURCC_iCCP:
      case FOURCC_tIME:
        // Ignore these chunks
        if (fseek(fp, chunk_length, SEEK_CUR))
          return close(fp, IE_IMAGE_FORMAT);
        break;
      case FOURCC_IDAT:
        if (n_IDAT_chunks == 255 || // Support PNGs with up to 255 IDAT chunks
            // Store file pointer offset & data length and seek past
            (png_info.PNG_data_offsets[n_IDAT_chunks] = (long)ftell(fp)) <= 0 ||
            !(png_info.PNG_data_lengths[n_IDAT_chunks] = chunk_length) ||
            fseek(fp, chunk_length, SEEK_CUR))
          return close(fp, IE_IMAGE_FORMAT);
        break;
      case FOURCC_IEND:
        if (!n_chunks || // Must not be first chunk
            chunk_length || // IEND Chunk must be empty
            !n_IDAT_chunks) // Must be at least 1 IDAT chunk before IEND
          return close(fp, IE_IMAGE_FORMAT);
        break;
      default:
        // Don't support any other chunk types
        return close(fp, IE_IMAGE_FORMAT);
    }
    if (fread(&chunk_crc, sizeof (uint), 1, fp) != 1)
      return close(fp, IE_IMAGE_CRC);
    if (chunk_id == FOURCC_IDAT)
      // Store and check later
      png_info.PNG_data_CRCs[n_IDAT_chunks++] = chunk_crc;
  }

  // Image width and height
  image->width = REVERSE32(png_info.IHDR.width);
  image->height = REVERSE32(png_info.IHDR.height);
  uint byte_width;

  // Pixel scheme and bytes per row
  switch (png_info.IHDR.colour_type) {
    case 2:
      image->pixel_scheme = PS_RGB;
      byte_width = image->width * (image->bpp = 24) / 8;
      break;
    case 3:
      image->pixel_scheme = PS_IRGB;
      image->bpp = 8;
      byte_width = image->width;
      break;
    case 6:
      image->pixel_scheme = PS_RGBA;
      byte_width = image->width * (image->bpp = 32) / 8;
      break;
    default:
      return close(fp, IE_IMAGE_FORMAT);
  }

  // Read compressed pixel data
  // Combine IDAT chunks
  long fpos = ftell(fp); // Store current file position
  uint i, in_data_length;
  for (i = 0, in_data_length = 0; i < n_IDAT_chunks; in_data_length += png_info.PNG_data_lengths[i++]);
  byte *in_data = malloc(in_data_length), *ptr = in_data;
  for (i = 0; i < n_IDAT_chunks; i++) {
    if (fseek(fp, png_info.PNG_data_offsets[i], SEEK_SET) ||
        fread(ptr, png_info.PNG_data_lengths[i], 1, fp) != 1) {
      free(in_data);
      return close(fp, IE_IMAGE_FORMAT);
    }
    ptr += png_info.PNG_data_lengths[i];
  }
  fseek(fp, fpos, SEEK_SET); // Restore file position

  byte *data;
  ulong data_length = ++byte_width * image->height; // Add extra byte (PNG spec)

  // Unzip
  z_stream zstream = { 0 };
  if (inflateInit(&zstream) != Z_OK)
    return close(fp, IE_IMAGE_UNZIP);
  zstream.next_in = in_data;
  zstream.avail_in = in_data_length;
  zstream.next_out = (data = malloc(data_length));
  int result;
  while (!zstream.avail_out) {
    zstream.avail_out = in_data_length;
    result = inflate(&zstream, Z_NO_FLUSH);
    if (!(result == Z_OK || result == Z_STREAM_END)) {
      free(data);
      data = NULL;
      break;
    }
  }
  inflateEnd(&zstream);
  free(in_data);

  if (!data)
    return close(fp, IE_IMAGE_UNZIP);

  // Convert from PNG layout to raster image
  // Remove 1 byte (filter method) from start of each row and apply filter to row
  image->byte_width = byte_width - 1;
  image->data_length = image->byte_width * image->height;
  image->data = malloc(image->data_length);
  byte p = image->bpp / 8;
  uint x, y;
  for (y = 0; y < image->height; y++) {
    memcpy(&image->data[y * image->byte_width], &data[y * byte_width + 1], image->byte_width);
    switch (data[y * byte_width]) {
      case 0: // None
        break;
      case 1: // Sub
        // Add previous pixel value
        for (x = p; x < image->byte_width; x++)
          image->data[y * image->byte_width + x] += image->data[y * image->byte_width + x - p];
        break;
      case 2: // Up
        // Add pixel values from previous scanline
        for (x = 0; x < image->byte_width; x++)
          image->data[y * image->byte_width + x] += image->data[(y - 1) * image->byte_width + x];
        break;
      case 3: // Average
        for (x = 0; x < p; x++)
          image->data[y * image->byte_width + x] += image->data[(y - 1) * image->byte_width + x] / 2;
        for (x = p; x < image->byte_width; x++)
          image->data[y * image->byte_width + x] += (image->data[(y - 1) * image->byte_width + x] + image->data[y * image->byte_width + x - p]) / 2;
        break;
      case 4: // Paeth
        for (x = 0; x < image->byte_width; x++);
        break;
    }
  }
  free(data);

  return close(fp, IE_OK);
}

void destroy_image(image_t *image) {
  if (image->data)
    free(image->data);
}
