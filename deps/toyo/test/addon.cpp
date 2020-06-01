#include <node/js_native_api.h>
napi_value create_addon(napi_env env);

#define NAPI_CALL(env, call)                                      \
  do {                                                            \
    napi_status status = (call);                                  \
    if (status != napi_ok) {                                      \
      const napi_extended_error_info* error_info = NULL;          \
      napi_get_last_error_info((env), &error_info);               \
      bool is_pending;                                            \
      napi_is_exception_pending((env), &is_pending);              \
      if (!is_pending) {                                          \
        const char* message = (error_info->error_message == NULL) \
            ? "empty error message"                               \
            : error_info->error_message;                          \
        napi_throw_error((env), NULL, message);                   \
        return NULL;                                              \
      }                                                           \
    }                                                             \
  } while(0)

#include <node/node_api.h>

#include "test.hpp"

napi_value run(napi_env env, napi_callback_info info) {
  int r = test();
  napi_value res;
  NAPI_CALL(env, napi_create_int32(env, r, &res));
  return res;
}

NAPI_MODULE_INIT() {
  napi_value js_run;
  NAPI_CALL(env, napi_create_function(env,
                                      "run",
                                      NAPI_AUTO_LENGTH,
                                      run,
                                      NULL,
                                      &js_run));
  NAPI_CALL(env, napi_set_named_property(env,
                                         exports,
                                         "run",
                                         js_run));

  return exports;
}
