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

/* Size of Elliptic Curve key */
#define ECDSA_KEY_SIZE 256

/* Size of a signature */
#define SIGNATURE_SIZE TEE_SHA256_HASH_SIZE * 2

/* Structure to keep track of the current session */
typedef struct video_ta_sess {
  TEE_OperationHandle op_handle; /* Handle to keep track of tee api op */
  TEE_ObjectHandle key_handle; /* Handle to keep track of key transient object */
  uint8_t ecdsa_pub_x_raw[ECDSA_KEY_SIZE / 8]; /* Elliptic pub key x */
  uint32_t ecdsa_pub_x_size;
  uint8_t ecdsa_pub_y_raw[ECDSA_KEY_SIZE / 8]; /* Elliptic pub key y */
  uint32_t ecdsa_pub_y_size;
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
static TEE_Result create_digest(video_ta_sess_t *sess_ctx,
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

/* Generate an ECDSA keypair.
 * Reference to a transient object containing the key
 * is put in sess_ctx->key_handle
 * Public key components are put in corresponding buffers
 * in sess_ctx struct
 */
TEE_Result gen_ecdsa_keypair(video_ta_sess_t *sess_ctx) 
{
  TEE_Result res = TEE_SUCCESS;
  TEE_Attribute curve_attr; // Required attribute for EC key gen

  /* Allocate transient object to hold key */
  res = TEE_AllocateTransientObject(TEE_TYPE_ECDSA_KEYPAIR, ECDSA_KEY_SIZE,
                                    &sess_ctx->key_handle);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to allocate transient object with error 0x%x", res);
    return res;
  }

  /* Initialize curve attribute for key gen */
  TEE_InitValueAttribute(&curve_attr, TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P256,
                         sizeof(int));

  /* Generate keypair */
  res = TEE_GenerateKey(sess_ctx->key_handle, ECDSA_KEY_SIZE, &curve_attr, 1);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to generate ECDSA keypair with error 0x%x", res);
    return res;
  }

  /* Set correct pubkey component sizes */
  sess_ctx->ecdsa_pub_x_size = sizeof(sess_ctx->ecdsa_pub_x_raw);
  sess_ctx->ecdsa_pub_y_size = sizeof(sess_ctx->ecdsa_pub_y_raw);

  /* Extract x component of pubkey */
  res = TEE_GetObjectBufferAttribute(sess_ctx->key_handle,
                                     TEE_ATTR_ECC_PUBLIC_VALUE_X,
                                     sess_ctx->ecdsa_pub_x_raw,
                                     &sess_ctx->ecdsa_pub_x_size);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to get pubkey x component with error 0x%x", res);
    return res;
  }

  /* Extract y component of pubkey */
  res = TEE_GetObjectBufferAttribute(sess_ctx->key_handle,
                                     TEE_ATTR_ECC_PUBLIC_VALUE_Y,
                                     sess_ctx->ecdsa_pub_y_raw,
                                     &sess_ctx->ecdsa_pub_y_size);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to get pubkey y component with error 0x%x", res);
    return res;
  }

  /* TEE_SUCCESS */
  return res;
}

TEE_Result sign_digest(video_ta_sess_t *sess_ctx, void *digest_buf
                       size_t digest_size);
{
  TEE_Result res = TEE_SUCCESS;

  /* Obtain ECDSA keypair for signing */
  res = gen_ecdsa_keypair(sess_ctx);
  if (res != TEE_SUCCESS) {
    return res;
  }

  /* Prepare signing operation */
  res = TEE_AllocateOperation(&sess_ctx->op_handle, TEE_ALG_ECDSA_P256,
                              TEE_MODE_SIGN, ECDSA_KEY_SIZE);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to allocate sign operaton with error 0x%x", res);
    return res;
  }

  /* Set the private key for signing */
  res = TEE_SetOperationKey(sess_ctx->op_handle, sess_ctx->key_handle);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to set key for signing with error 0x%x", res);
    return res;
  }

  uint8_t signature_buf[SIGNATURE_SIZE];

  /* Will verify sig len after signing */
  uint32_t signature_size = SIGNATURE_SIZE;
  uint32_t exp_signature_size = SIGNATURE_SIZE;

  /* Perform signing operation */
  res = TEE_AsymmetricSignDigest(sess_ctx->op_handle, NULL, 0, digest_buf,
                                 digest_size, signature_buf, &signature_size);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to sign digest with error 0x%x", res);
    return res;
  }

  if (signature_size != exp_signature_size) {
    EMSG("Signature length was incorrect!");
    return TEE_ERROR_GENERIC;
  }
}

/* Increment the value and sign the result */
static TEE_Result inc_and_sign(video_ta_sess_t *sess_ctx,
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

  res = create_digest(sess_ctx, val_ref, sizeof(uint32_t), params[1].memref.buffer);

  /* Sign the hashed value */

  /* Free operation and exit */
  TEE_FreeOperation(sess_ctx->op_handle);
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
      return inc_and_sign(sess_ctx, param_types, params);
    default:
      return TEE_ERROR_BAD_PARAMETERS;
  }
}
