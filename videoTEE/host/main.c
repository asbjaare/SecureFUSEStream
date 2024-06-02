/*
 * Client of the video processing program running in the
 * TEE of an ARM TrustZone CPU.
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OP-TEE client API for communicating with the TA */
#include <tee_client_api.h>

/* TA's header file */
#include <video_tee_ta.h>

/* BMP library */
#include "bmp.h"
#include "libbmp.h"

/* Size of buffer to receive hash */
#define DIGEST_SIZE (256 / 8)

/* Timer helper courtesy of Morten Grønnesby */
unsigned long long gettime()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1)
    {
        fprintf(stderr, "Could not get time\n");
        return -1;
    }

    unsigned long long micros = 1000000 * tv.tv_sec + tv.tv_usec;

    return micros;
}

/* Helper to print hex values */
void print_hex(uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("%02x", buf[i]);
  }
  printf("\n");
}

/* Load an image into memory */
FILE *load_img(char *path, RGB **img, img_meta_t *metadata) {

  /* Open img file */
  FILE *img_file = fopen(path, "rb");

  /* Read file contents */
	bmp_img img_handle;
	bmp_img_read(&img_handle, img_file);

  /* Get image dimensions */
  int width, height;
  GetSize(img_handle, &width, &height);

  /* Allocate image */
  *img = malloc(sizeof(RGB) * width * height);
  if (*img == NULL)
    return img_file;

  /* Load from bmp */
  LoadRegion(img_handle, 0, 0, width, height, *img);

  /* Set metadata */
  (metadata->width) = (uint32_t)width;
  (metadata->height) = (uint32_t)height;

  bmp_img_free(&img_handle);

  return img_file;
}

/* Write an image from memory to disk */
void write_img(char *path, RGB *img, img_meta_t *metadata) {
  /* Create BMP file */
  CreateBMP(path, metadata->width, metadata->height);
  /* Write image data */
  WriteRegion(path, 0, 0, metadata->width, metadata->height, img);
}

int main(int argc, char *argv[]) {
  /* OP-TEE constructs */
  TEEC_UUID uuid = TA_VIDEO_TEE_UUID;
  TEEC_Result res;
  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  uint32_t err_origin;
  RGB *res_img;
  RGB *img;
  signed_res_t *res_buf;
  FILE *img_fp;


  /* Load image into memory */
  img_meta_t *metadata = calloc(1, sizeof(img_meta_t));
  img_fp = load_img(argv[1], &img, metadata);
  if (img == NULL)
    errx(EXIT_FAILURE, "failed to load image");

  /* Get time before operation */
  unsigned long long t_start = gettime();

  /* Connect to TEE */
  res = TEEC_InitializeContext(NULL, &ctx);
  if (res != TEEC_SUCCESS)
    errx(EXIT_FAILURE, "Failed to initialize TEE Context with code 0x%x", res);

  /* Open a session to connect to the TA */
  res = TEEC_OpenSession(&ctx, &sess, &uuid, TEEC_LOGIN_PUBLIC, NULL, NULL,
                         &err_origin);
  if (res != TEEC_SUCCESS) {
    errx(EXIT_FAILURE,
         "Failed to open session to TA with code 0x%x, origin 0x%x", res,
         err_origin);
  }

  /* Clear operation struct */
  memset(&op, 0, sizeof(op));

  /* Insert argument for TA invocation. */
  op.paramTypes =
      TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_OUTPUT,
                       TEEC_MEMREF_TEMP_OUTPUT, TEEC_NONE);

  /* Load image into memref to send to TA */
  op.params[0].tmpref.size =
      (size_t)(sizeof(RGB) * metadata->width * metadata->height);
  op.params[0].tmpref.buffer = img;

  /* Initialize output memref parameters */
  op.params[1].tmpref.size = (size_t)sizeof(signed_res_t);
  res_buf = calloc(1, sizeof(signed_res_t));
  if (res_buf == NULL)
    errx(EXIT_FAILURE, "Failed to allocate buffer for digest");
  op.params[1].tmpref.buffer = res_buf;

  op.params[2].tmpref.size = sizeof(RGB) * metadata->width * metadata->height;
  res_img = calloc(1, sizeof(RGB) * metadata->width * metadata->height);
  if (res_img == NULL)
    errx(EXIT_FAILURE, "Failed to allocate buffer for result image");
  op.params[2].tmpref.buffer = res_img;

  /*
   * Invoke TA to increment number, hash the result and
   * sign it with its hardware key
   */
  // printf("Invoking TA to process image and sign the operation\n");
  res = TEEC_InvokeCommand(&sess, TA_VIDEO_INC_SIGN, &op, &err_origin);
  // printf("Success\n");
  if (res != TEEC_SUCCESS)
    errx(EXIT_FAILURE, "TA invocation failed with code 0x%x, origin 0x%x", res,
         err_origin);

  // Print timestamp of successful computation
  unsigned long long t_finish = gettime();
  // printf("Finished at: %lld\n", t_finish);
  printf("Took: %lld\n", (t_finish - t_start) / 1000);
  fclose(img_fp);
  // char new_filename[256]; // Adjust size as needed based on the maximum expected
                          // path length
  /* Write processed image to disk */
  // sprintf(new_filename, "gray%s", argv[1]);
  // write_img(new_filename, res_img, metadata);

  /* ------------------- PRINTS ------------------- */

  // printf("Hash of result: ");
  // print_hex(res_buf->digest, DIGEST_SIZE);
  //
  // printf("Signed hash: ");
  // print_hex(res_buf->signature, SIGNATURE_SIZE);
  //
  // printf("Pubkey x component: ");
  // print_hex(res_buf->pub_key_x, res_buf->pub_key_x_size);
  //
  // printf("Pubkey y component: ");
  // print_hex(res_buf->pub_key_y, res_buf->pub_key_y_size);

  /* Cleanup session and context */
  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  return EXIT_SUCCESS;
}
