#include "ngx_script.h"
#include <cassert>

namespace ngx_opentracing {
//------------------------------------------------------------------------------
// NgxScript
//------------------------------------------------------------------------------
NgxScript::NgxScript()
    : pattern_{0, nullptr}, lengths_{nullptr}, values_{nullptr} {}

//------------------------------------------------------------------------------
// compile
//------------------------------------------------------------------------------
ngx_int_t NgxScript::compile(ngx_conf_t *cf, const ngx_str_t &pattern) {
  pattern_ = pattern;
  lengths_ = nullptr;
  values_ = nullptr;

  auto num_variables = ngx_http_script_variables_count(&pattern_);
  // As an optimization, don't compile the script if there are no variables.
  if (num_variables == 0)
    return NGX_OK;

  ngx_http_script_compile_t script_compile;
  ngx_memzero(&script_compile, sizeof(ngx_http_script_compile_t));
  script_compile.cf = cf;
  script_compile.source = &pattern_;
  script_compile.lengths = &lengths_;
  script_compile.values = &values_;
  script_compile.variables = num_variables;
  script_compile.complete_lengths = 1;
  script_compile.complete_values = 1;

  return ngx_http_script_compile(&script_compile);
}

//------------------------------------------------------------------------------
// run
//------------------------------------------------------------------------------
ngx_str_t NgxScript::run(ngx_http_request_t *request) const {
  assert(is_valid());

  // If the script has no variables, we can just return the pattern.
  if (!lengths_)
    return pattern_;

  ngx_str_t result = {0, nullptr};
  if (!ngx_http_script_run(request, &result, lengths_->elts, 0,
                           values_->elts)) {
    ngx_log_error(NGX_LOG_ERR, request->connection->log, 0,
                  "failed to run script");
    return {0, nullptr};
  }
  return result;
}
} // namespace ngx_opentracing