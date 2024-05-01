/*
 * Trusted application for processing video inside the TEE
 * of an ARM TrustZone chip.
 *
 * Current version simply increments a number it receives from the client,
 * hashes the resulting value, and signs this hash with the hardware key
 */

/* TEE api to use TEE features */
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include <video_tee_ta.h>

/* Digest algorithm to use */
#define DIGEST_ALG TEE_ALG_SHA256

/* Size of digest (using SHA256) */
#define DIGEST_SIZE (256 / 8)

/* Structure to keep track of the current session */
typedef struct video_ta_sess {
  TEE_OperationHandle op_handle; /* Handle to keep track of tee api op */
  TEE_ObjectHandle key_handle; /* Handle to keep track of hwkey(?) */
} video_ta_sess_t;

/* Called when TA instance is created */
TEE_Result TA_CreateEntryPoint() 
{
  DMSG("Hello from video tee TA!");

  return TEE_SUCCESS;
}

/* Called when the TA instance is destroyed */
void TA_DestroyEntryPoint() 
{
  DMSG("Goodbye from video tee TA");
}

/* Initialize a session with a client */
TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types,
                                    TEE_Param __unused params[4],
                                    void __unused **session) 
{
  /* Allocate session struct */
  video_ta_sess_t *sess_ctx;

  sess_ctx = TEE_Malloc(sizeof(*sess_ctx), TEE_MALLOC_FILL_ZERO);
  if (!sess_ctx)
    return TEE_ERROR_OUT_OF_MEMORY;

  *session = (void *)sess_ctx;

  return TEE_SUCCESS;
}

/* Close the client session */
void TA_CloseSessionEntryPoint(void *session) 
{
  video_ta_sess_t *sess_ctx = (video_ta_sess_t *)session;

  /* Free operation */
  if (sess_ctx->op_handle != TEE_HANDLE_NULL)
    TEE_FreeOperation(sess_ctx->op_handle);

  /* Free session */
  TEE_Free(sess_ctx);
}

/* Create a digest of the given data */
static TEE_Result CreateDigest(video_ta_sess_t *sess_ctx,
                               void *in_buf, size_t in_size,
                               void *out_buf)
{
  TEE_Result res = TEE_SUCCESS;

  /* Prepare digest operation */
  res = TEE_AllocateOperation(&sess_ctx->op_handle,
                              DIGEST_ALG,
                              TEE_MODE_DIGEST,
                              0); /* No key used for digest */
  if (res != TEE_SUCCESS) {
    EMSG("Failed to allocate digest operation");
    sess_ctx->op_handle = TEE_HANDLE_NULL;
    return res;
  }

  /* Perform digest operation */
  uint32_t digest_size = DIGEST_SIZE;
  res = TEE_DigestDoFinal(sess_ctx->op_handle,
                          in_buf, in_size,
                          out_buf, &digest_size);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to perform digest operation");
  }

  return res;
}

/* Increment the value and sign the result */
static TEE_Result incAndSign(video_ta_sess_t *sess_ctx,
                             uint32_t param_types, TEE_Param params[4]) 
{
  TEE_Result res = TEE_SUCCESS;
  /* Expected parameter types */
  uint32_t exp_param_types = TEE_PARAM_TYPES(
    TEE_PARAM_TYPE_VALUE_INOUT, /* Value to increment */
    TEE_PARAM_TYPE_MEMREF_OUTPUT, /* Value digest */
    TEE_PARAM_TYPE_NONE,
    TEE_PARAM_TYPE_NONE);

  if (param_types != exp_param_types)
    return TEE_ERROR_BAD_PARAMETERS;

  /* Increment the value */
  params[0].value.a++;

  /* Generate a hash of the new value */
  uint32_t val = params[0].value.a;
  void *val_ref = (void *)&val;

  params[1].memref.size = DIGEST_SIZE;

  res = CreateDigest(sess_ctx, val_ref, sizeof(uint32_t), params[1].memref.buffer);

  /* Sign the hashed value */

  return res;
}

/* Entry point to invoke a specified command */
TEE_Result TA_InvokeCommandEntryPoint(
  void *session,
  uint32_t cmd_id,
  uint32_t param_types,
  TEE_Param params[4]) 
{
  /* Unused parameter for now */
  video_ta_sess_t *sess_ctx = (video_ta_sess_t *)session;

  switch (cmd_id) {
    case TA_VIDEO_INC_SIGN:
      return incAndSign(sess_ctx, param_types, params);
    default:
      return TEE_ERROR_BAD_PARAMETERS;
  }
}
