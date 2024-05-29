/*
 * Client of the video processing program running in the
 * TEE of an ARM TrustZone CPU.
 */

#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* OP-TEE client API for communicating with the TA */
#include <tee_client_api.h>

/* TA's header file */
#include <video_tee_ta.h>

/* BMP library */
// #include <libbmp.h>
#include "bmp.h"

/* Size of buffer to receive hash */
#define DIGEST_SIZE (256 / 8)

/* Helper to print hex values */
void print_hex(uint8_t *buf, size_t len)
{
  for (size_t i = 0; i < len; i++) {
      printf("%02x", buf[i]);
  }
  printf("\n");
}

/* Load an image into memory */
void load_img(char *path, RGB *img, img_meta_t *metadata)
{
  /* Get image dimensions */
  int width, height;
  GetSize(path, &width, &height);

  /* Allocate image */
  img = malloc(sizeof(RGB) * width * height);

  /* Load from bmp */
  LoadRegion(path, 0, 0, width, height, img);

  /* Set metadata */
  (metadata->width) = width;
  (metadata->height) = height;

  return;
}

/* Write an image from memory to disk */
void write_img(char *path, RGB *img, img_meta_t *metadata)
{
  /* Create BMP file */
  CreateBMP(path, metadata->width, metadata->height);
  /* Write image data */
  WriteRegion(path, 0, 0, metadata->width, metadata->height, img);
}

int main(void)
{
  /* OP-TEE constructs */
  TEEC_UUID uuid = TA_VIDEO_TEE_UUID;
  TEEC_Result res;
  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  uint32_t err_origin;
  signed_res_t *res_buf;

  /* Connect to TEE */
  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS)
    errx(EXIT_FAILURE, "Failed to initialize TEE Context with code 0x%x", res);

  /* Open a session to connect to the TA */
  res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, 
                         NULL, NULL, &err_origin);
  if (res != TEEC_SUCCESS) {
    errx(EXIT_FAILURE, "Failed to open session to TA with code 0x%x, origin 0x%x",
         res, err_origin);
  }

  /* Clear operation struct */
  memset(&op, 0, sizeof(op));

  /* Insert argument for TA invocation. */
  op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT,
                                   TEEC_MEMREF_TEMP_OUTPUT, TEEC_MEMREF_TEMP_OUTPUT);

  /* Load image into memref to send to TA */
  img_meta_t *metadata = calloc(1, sizeof(img_meta_t));
  load_img("cod.bmp", op.params[0].tmpref.buffer, metadata);
  op.params[0].tmpref.size = sizeof(RGB) * metadata->width * metadata->height;
  op.params[1].tmpref.buffer = metadata;
  op.params[1].tmpref.size = sizeof(metadata);

  /* Initialize output memref parameters */
  op.params[3].tmpref.size = (size_t)sizeof(signed_res_t);
  res_buf = calloc(1, sizeof(signed_res_t));
  if (res_buf == NULL)
    errx(EXIT_FAILURE, "Failed to allocate buffer for digest");
  op.params[3].tmpref.buffer = res_buf;

  /*
   * Invoke TA to increment number, hash the result and
   * sign it with its hardware key
   */
  printf("Invoking TA to process image and sign the operation\n");
  res = TEEC_InvokeCommand(&sess, TA_VIDEO_INC_SIGN, &op, &err_origin);
  if (res != TEEC_SUCCESS)
    errx(EXIT_FAILURE, "TA invocation failed with code 0x%x, origin 0x%x",
         res, err_origin);

  /* Write processed image to disk */
  write_img("cod_grayscale.bmp", op.params[2].tmpref.buffer, metadata);

  /* ------------------- PRINTS ------------------- */
  // printf("Result of operation: %d\n", op.params[0].value.a);

  // printf("Result in result struct: %d\n", res_buf->val);

  printf("Hash of result: ");
  print_hex(res_buf->digest, DIGEST_SIZE);

  printf("Signed hash: ");
  print_hex(res_buf->signature, SIGNATURE_SIZE);

  printf("Pubkey x component: ");
  print_hex(res_buf->pub_key_x, res_buf->pub_key_x_size);

  printf("Pubkey y component: ");
  print_hex(res_buf->pub_key_y, res_buf->pub_key_y_size);

  /* Cleanup session and context */
  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  return EXIT_SUCCESS;
}
