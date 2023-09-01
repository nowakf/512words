typedef struct occupant occupant;

void get_occupant_probability(occupant *occupants, const uint8_t *buf, int len);
void closest_transformation(occupant *probable_occupants, const uint8_t *in, uint8_t *out, int len);

//returns the closest utf8 to the arbitrary bytes supplied
void to_utf8(char * buf, size_t buf_len);

//returns the difference between the original and converted bytes
void diff(char * original, char * utf8, size_t len);

//cuts non-displaying control characters
void cut_problem_control_chars(char * string, size_t len);

//downsizes an image by a factor of two, writes the shrunken image 
//in the original buffer
void shrink(uint8_t *luma, int w, int h);

//thresholds an image by the supplied 'mid', writing one bit for
//each thresholded byte in the supplied buffer
void threshold(uint8_t * luma, int len, uint8_t mid);
