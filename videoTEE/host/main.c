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

/* Size of buffer to receive hash */
#define DIGEST_SIZE (256 / 8)

int main(void)
{
  /* OP-TEE constructs */
  TEEC_UUID uuid = TA_VIDEO_TEE_UUID;
  TEEC_Result res;
  TEEC_Context ctx;
  TEEC_Session sess;
  TEEC_Operation op;
  uint32_t err_origin;

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

  /* Insert argument for TA invocation. Just send a number for now */
  op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INOUT, TEEC_MEMREF_TEMP_OUTPUT,
                                   TEEC_NONE, TEEC_NONE);
  op.params[0].value.a = 69;

  /* Initialize memref parameters */
  op.params[1].tmpref.size = (size_t)DIGEST_SIZE;

  /*
   * Invoke TA to increment number, hash the result and
   * sign it with its hardware key
   */
  printf("Invoking TA to increment %d and sign the operation\n",
         op.params[0].value.a);
  res = TEEC_InvokeCommand(&sess, TA_VIDEO_INC_SIGN, &op, &err_origin);
  if (res != TEEC_SUCCESS) {
    errx(EXIT_FAILURE, "TA invocation failed with code 0x%x, origin 0x%x",
         res, err_origin);
  }
  printf("Result of operation: %d\n Hash of result: %s\n",
         op.params[0].value.a, (char *)op.params[1].tmpref.buffer);


  /* Cleanup session and context */
  TEEC_CloseSession(&sess);
  TEEC_FinalizeContext(&ctx);

  return EXIT_SUCCESS;
}
