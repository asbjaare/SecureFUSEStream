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

/* Structure to keep track of the current session */
typedef struct video_ta_sess {
  TEE_OperationHandle op_handle; /* Handle to keep track of tee api op */
  TEE_ObjectHandle key_handle; /* Handle to keep track of key transient object */
  signed_res_t res; // The signed result to return to client
} video_ta_sess_t;

typedef struct RGB {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} RGB;

/* Convert an image to grayscale
 * COURTESY OF UIT. TAKEN FROM THE PRECODE IN THE PARALLEL PROGRAMMING COURSE */
void ImageToGrayscale(RGB *img, size_t size)
{
    for(int i = 0; i < size; i++){
        char grayscale = img[i].red * 0.3 + img[i].green * 0.59 + img[i].blue * 0.11;
        img[i].red = grayscale;
        img[i].green = grayscale;
        img[i].blue = grayscale;
    }
}

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
                                    void **session)
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

  /* Free key handle */
  TEE_FreeTransientObject(sess_ctx->key_handle);

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

  /* Set initial pubkey component sizes */
  sess_ctx->res.pub_key_x_size = sizeof(sess_ctx->res.pub_key_x);
  sess_ctx->res.pub_key_y_size = sizeof(sess_ctx->res.pub_key_y);

  /* Extract x component of pubkey */
  res = TEE_GetObjectBufferAttribute(sess_ctx->key_handle,
                                     TEE_ATTR_ECC_PUBLIC_VALUE_X,
                                     sess_ctx->res.pub_key_x,
                                     &sess_ctx->res.pub_key_x_size);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to get pubkey x component with error 0x%x", res);
    return res;
  }

  /* Extract y component of pubkey */
  res = TEE_GetObjectBufferAttribute(sess_ctx->key_handle,
                                     TEE_ATTR_ECC_PUBLIC_VALUE_Y,
                                     sess_ctx->res.pub_key_y,
                                     &sess_ctx->res.pub_key_y_size);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to get pubkey y component with error 0x%x", res);
    return res;
  }

  /* TEE_SUCCESS */
  return res;
}

TEE_Result sign_digest(video_ta_sess_t *sess_ctx)
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

  /* Will verify sig len after signing */
  uint32_t signature_size = sizeof(sess_ctx->res.signature);
  uint32_t exp_signature_size = SIGNATURE_SIZE;

  /* Perform signing operation */
  res = TEE_AsymmetricSignDigest(sess_ctx->op_handle, NULL, 0, sess_ctx->res.digest,
                                 sizeof(sess_ctx->res.digest),
                                 sess_ctx->res.signature,
                                 &signature_size);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to sign digest with error 0x%x", res);
    return res;
  }

  if (signature_size != exp_signature_size) {
    EMSG("Signature length was incorrect!");
    return TEE_ERROR_GENERIC;
  }

  return res;
}

/* Save to secure storage */
static TEE_Result save_secure(video_ta_sess_t *sess_ctx, void *data, size_t data_size)
{
  TEE_Result res = TEE_SUCCESS;
  TEE_ObjectHandle obj_handle;

  size_t obj_id_size = DIGEST_SIZE;
  char *obj_id = TEE_Malloc(obj_id_size, 0);
  if (obj_id == NULL)
    return TEE_ERROR_OUT_OF_MEMORY;

  TEE_MemMove(obj_id, sess_ctx->digest, obj_id_size);

	uint32_t obj_data_flag = TEE_DATA_FLAG_ACCESS_READ |		/* we can later read the oject */
                           TEE_DATA_FLAG_ACCESS_WRITE |		/* we can later write into the object */
                           TEE_DATA_FLAG_ACCESS_WRITE_META |	/* we can later destroy or rename the object */
                           TEE_DATA_FLAG_OVERWRITE;		/* destroy existing object of same ID */

  res = TEE_CreatePersistentObject(TEE_STORAGE_PRIVATE,
                                   obj_id, obj_id_size,
                                   obj_data_flag,
                                   TEE_HANDLE_NULL,
                                   NULL, 0,
                                   &obj_handle);
  if (res != TEE_SUCCESS) {
    EMSG("Failed to creat persistent object 0x%08x", res);
    return res;
  }

  res = TEE_WriteObjectData(obj_handle, data, data_size);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_WriteObjectData failed 0x%08x", res);
		TEE_CloseAndDeletePersistentObject1(obj_handle);
	} else {
		TEE_CloseObject(obj_handle);
	}

  TEE_Free(obj_id);
  return res;
}

/* Increment the value and sign the result */
static TEE_Result inc_and_sign(video_ta_sess_t *sess_ctx,
                             uint32_t param_types, TEE_Param params[4])
{
  TEE_Result res = TEE_SUCCESS;

  /* Expected parameter types */
  uint32_t exp_param_types = TEE_PARAM_TYPES(
    TEE_PARAM_TYPE_MEMREF_INPUT, /* Input image */
    TEE_PARAM_TYPE_MEMREF_OUTPUT, /* Attestation */
    TEE_PARAM_TYPE_MEMREF_OUTPUT, /* Out img */
    TEE_PARAM_TYPE_NONE);

  if (param_types != exp_param_types)
    return TEE_ERROR_BAD_PARAMETERS;

  RGB *img = TEE_Malloc(params[0].memref.size, TEE_MALLOC_FILL_ZERO);
  if (img == NULL)
    return TEE_ERROR_OUT_OF_MEMORY;
  TEE_MemMove(img, (RGB *)params[0].memref.buffer, (size_t)params[0].memref.size);

  ImageToGrayscale(img, (params[0].memref.size)/sizeof(RGB));

  /* Generate a hash of the new image */
  res = create_digest(sess_ctx, img, sizeof(uint32_t), &(sess_ctx->res.digest));
  if (res != TEE_SUCCESS) {
    EMSG("Failed to create digest with error 0x%x", res);
    return res;
  }

  /* Sign the hashed value */
  res = sign_digest(sess_ctx);
  if (res != TEE_SUCCESS)
    EMSG("Failed to sign digest with error 0x%x", res);

  /* Copy attestation results into return buffer */
  params[1].memref.size = sizeof(sess_ctx->res);
  TEE_MemMove(params[1].memref.buffer, &sess_ctx->res, sizeof(sess_ctx->res));

  /* Copy processed image */
  params[2].memref.size = params[0].memref.size;
  TEE_MemMove(params[2].memref.buffer, img, params[0].memref.size);

  /* Free operation and exit */
  TEE_FreeOperation(sess_ctx->op_handle); // NOTE: Is this needed here?
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
