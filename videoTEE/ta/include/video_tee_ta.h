#ifndef VIDEO_TEE_TA_H
#define VIDEO_TEE_TA_H


/* UUID of the TEE video processor trusted application */
#define TA_VIDEO_TEE_UUID \
    { 0x236268e6, 0xa7a4, 0x4bcd, \
        { 0x97, 0xc6, 0x45, 0x1e, 0xbd, 0x80, 0x2b, 0xeb } }

/* Operations */
#define TA_VIDEO_INC_SIGN 0

/* Size of digest (using SHA256) */
#define DIGEST_SIZE (256 / 8)

/* Size of Elliptic Curve key */
#define ECDSA_KEY_SIZE 256
#define ECDSA_KEY_SIZE_BYTES ECDSA_KEY_SIZE / 8

/* Size of a signature */
#define SIGNATURE_SIZE DIGEST_SIZE * 2

/* Image metadata */
typedef struct img_meta {
  uint32_t width;
  uint32_t height;
} img_meta_t;

/* Structure of output data */
typedef struct signed_res {
  uint8_t digest[DIGEST_SIZE];
  uint8_t signature[SIGNATURE_SIZE]; // Signed digest
  uint8_t pub_key_x[ECDSA_KEY_SIZE_BYTES]; // Pub key for verification
  uint32_t pub_key_x_size; // Component size can vary (?)
  uint8_t pub_key_y[ECDSA_KEY_SIZE_BYTES];
  uint32_t pub_key_y_size;
} signed_res_t;

#endif // !VIDEO_TEE_TA_H
