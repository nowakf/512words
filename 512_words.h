//crops an image. 
//image w and h must be lower than the crop w and h
void crop(uint8_t * luma, int prev_w, int w, int h);

//thresholds an image by the supplied 'mid', writing one bit for
//each thresholded byte in the supplied buffer
void threshold(const uint8_t * luma, uint8_t * bits, int len, uint8_t mid);

//desaturates an image
void rgba2luma(uint8_t * luma, int len);

//composites two bitfields into a rg8 image
void composite(uint8_t * rg8, const uint8_t * a, const uint8_t *b, size_t len);

//returns the closest utf8 to the arbitrary bytes supplied
uint8_t * to_utf8(const uint8_t * in, size_t buf_len);

//returns the difference between the original and converted bytes
uint8_t * diff(const uint8_t * original, const uint8_t * utf8, size_t len);

//encodes a png -- useful for debugging. caller must free buf
int encode_png(uint8_t * png_buf, size_t *png_size, uint8_t *bitmap, int w, int h);
