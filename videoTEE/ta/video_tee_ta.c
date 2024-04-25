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

/* Called when TA instance is created */
TEE_Result TA_CreateEntryPoint() {
  DMSG("Hello from video tee TA!");

  return TEE_SUCCESS;
}

/* Called when the TA instance is destroyed */
void TA_DestroyEntryPoint() {
  DMSG("Goodbye from video tee TA");
}

/* Initialize a session with a client */
TEE_Result TA_OpenSessionEntryPoint(uint32_t param_types,
                                    TEE_Param __maybe_unused params[4],
                                    void __maybe_unused **sess_ctx) {
  /* Expected parameter types */
  uint32_t exp_param_types = TEE_PARAM_TYPES(TEE_PARAM_TYPE_NONE,
                                             TEE_PARAM_TYPE_NONE,
                                             TEE_PARAM_TYPE_NONE,
                                             TEE_PARAM_TYPE_NONE);

  if (param_types != exp_param_types)
    return TEE_ERROR_BAD_PARAMETERS;

  /* Unused parameters for now */
  (void)&params;
  (void)&sess_ctxp;

  return TEE_SUCCESS;
}

/* Close the client session */
void TA_CloseSessionEntryPoint(void __maybe_unused *sess_ctx) {
  /* Unused parameter for now */
  (void)&sess_ctx;
}

/* Increment the value and sign the result */
static TEE_Result incAndSign(uint32_t param_types, TEE_Param params[4]) {
  /* Expected parameter types */
  uint32_t exp_param_types = TEE_PARAM_TYPES(
    TEE_PARAM_TYPE_VALUE_INOUT, /* Value to increment */
    TEE_PARAM_TYPE_NONE,
    TEE_PARAM_TYPE_NONE,
    TEE_PARAM_TYPE_NONE,
  )
  if (param_types != exp_param_types)
    return TEE_ERROR_BAD_PARAMETERS;

  /* Increment the value */
  params[0].value.a++;

  /* Generate a hash of the new value */

  /* Sign the hashed value */

  return TEE_SUCCESS;
}

/* Entry point to invoke a specified command */
TEE_Result TA_InvokeCommandEntryPoint(
  void __maybe_unused *sess_ctx,
  uint32_t cmd_id,
  uint32_t param_types,
  TEE_Param params[4]) {
  /* Unused parameter for now */
  (void)&sess_ctx;

  switch (cmd_id) {
    case TA_VIDEO_INC_SIGN:
      return incAndSign(param_types, params);
    default:
      return TEE_ERROR_BAD_PARAMETERS;
  }
}
