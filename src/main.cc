#include "third_party/lodepng/lodepng.h"
#include <iostream>
#include <cassert>


// splits an (n*128) x (m*128) PNG up into 128x128 sub-images in row-major order
// like this (each digit representes 32x32 pixels):
//
// 0000111122223333
// 0000111122223333
// 0000111122223333
// 4444555566667777
// 4444555566667777
// 4444555566667777
// 4444555566667777
//
// this is useful for making larger slack macro emojis

int main(int argc, char** argv) {
  if(argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <png file to split>\n";
    return -1;
  }

  // load the PNG from a file
  std::vector<unsigned char> image; //the raw pixels
  unsigned width, height;
  const std::string filename = std::string(argv[1]);
  unsigned error = lodepng::decode(image, width, height, filename);
  if(error) {
    std::cerr << "decoder error " << error << ": " << lodepng_error_text(error) << "\n";
    return -1;
  }

  const unsigned kDim = 128;

  // its dimensions must be a multiple of 128
  if(width % kDim != 0 || height % kDim != 0) {
    std::cerr << "width/height must be a multiple of 128\n";
    return -1;
  }

  const unsigned kBytesPerPixel = 4;
  const unsigned kBytesPerSubimgRow = kBytesPerPixel*kDim;
  const unsigned kBytesPerSubimg = kDim*kDim*kBytesPerPixel;
  const unsigned kSubimgsPerY = height/kDim;
  const unsigned kSubimgsPerX = width/kDim;
  const unsigned kNumSubimgs = kSubimgsPerY*kSubimgsPerX;

  // create the subimage raw data arrays
  std::vector<std::vector<unsigned char>> subimgs;
  subimgs.reserve(kNumSubimgs);
  for(unsigned i = 0; i < kNumSubimgs; i++) {
    std::vector<unsigned char> subimg(kBytesPerSubimg);
    subimgs.emplace_back(subimg);
  }

  // copy raw pixel data from image to subimgs
  for(unsigned i = 0; i < image.size(); i += kBytesPerSubimgRow) {
    auto in = image.begin() + i;

    // calculate the x,y index of the subimg for the pixel pointed at by i
    const auto kSubimgX = (i / kBytesPerSubimgRow) % kSubimgsPerX;
    const auto kSubimgY = (i / (kBytesPerSubimgRow*kBytesPerSubimgRow));
    assert(kSubimgX < kSubimgsPerX);
    assert(kSubimgY < kSubimgsPerY);

    // from the (x,y) for the subimg, get its index in subimgs
    const auto kSubimgIndex = kSubimgX + kSubimgY*kSubimgsPerX;
    assert(kSubimgIndex < subimgs.size());
    auto& subimg = subimgs[kSubimgIndex];

    // figure out which row we're talking about inside the subimg
    auto out_r = (i/(width*kBytesPerPixel)) % 128;
    assert(out_r < 128);
    auto out = subimg.begin() + out_r*kBytesPerSubimgRow;

    std::copy(in, in + kBytesPerSubimgRow, out);
  }

  // write the output
  auto newfilename_base = filename.substr(0, filename.size() - 3);
  for(unsigned i = 0; i < subimgs.size(); i++) {
    auto newfilename = newfilename_base + std::to_string(i) + ".png";
    std::cerr << "writing " << newfilename << "\n";
    auto error = lodepng::encode(newfilename, subimgs[i], kDim, kDim);
    if(error) {
      std::cerr << "encoder error " << error << ": " << lodepng_error_text(error) << "\n";
      return -1;
    }
  }

  for(unsigned i = 0; i < subimgs.size(); i++) {
    std::cout << ":thing" << i << ":";
    if ((i + 1) % kSubimgsPerX == 0) {
      std::cout << "\n";
    }
  }
}
