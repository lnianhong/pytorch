#include "ATen/ATen.h"
#include "ATen/Dispatch.h"
#include "ATen/ExpandUtils.h"
#include "ATen/NativeFunctions.h"
#include "ATen/WrapDimUtils.h"
#include "ATen/WrapDimUtilsMulti.h"
#include "cpu/ReduceOpsKernel.h"

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

#include <map>

namespace at {
namespace native {

static inline Tensor integer_upcast(const Tensor& self, optional<ScalarType> dtype) {
  ScalarType scalarType = self.type().scalarType();
  ScalarType upcast_scalarType = dtype.value_or(at::isIntegralType(scalarType) ? ScalarType::Long : scalarType);
  return self.toType(upcast_scalarType);
}

static inline Tensor cumsum(const Tensor& self, int64_t dim, optional<ScalarType> dtype) {
  return at::_cumsum(integer_upcast(self, dtype), dim);
}

Tensor cumsum(const Tensor& self, int64_t dim, ScalarType dtype) {
  return at::native::cumsum(self, dim, optional<ScalarType>(dtype));
}

Tensor cumsum(const Tensor& self, int64_t dim) {
  return at::native::cumsum(self, dim, nullopt);
}

static inline Tensor& cumsum_out(Tensor& result, const Tensor& self, int64_t dim, optional<ScalarType> dtype) {
  // result type is favored over dtype; check that they match if provided (NumPy doesn't check)
  AT_CHECK(!dtype.has_value() || (result.type().scalarType() == dtype.value()),
            "provided dtype must match dtype of result in cumsum.  Got %s and %s.",
            at::toString(result.type().scalarType()), at::toString(dtype.value()));
  return at::_cumsum_out(result, self.toType(result.type().scalarType()), dim);
}

Tensor& cumsum_out(Tensor& result, const Tensor& self, int64_t dim, ScalarType dtype) {
  return at::native::cumsum_out(result, self, dim, optional<ScalarType>(dtype));
}

Tensor& cumsum_out(Tensor& result, const Tensor& self, int64_t dim) {
  return at::native::cumsum_out(result, self, dim, nullopt);
}

static inline Tensor cumprod(const Tensor& self, int64_t dim, optional<ScalarType> dtype) {
  return at::_cumprod(integer_upcast(self, dtype), dim);
}

Tensor cumprod(const Tensor& self, int64_t dim, ScalarType dtype) {
  return at::native::cumprod(self, dim, optional<ScalarType>(dtype));
}

Tensor cumprod(const Tensor& self, int64_t dim) {
  return at::native::cumprod(self, dim, nullopt);
}

static inline Tensor& cumprod_out(Tensor& result, const Tensor& self, int64_t dim, optional<ScalarType> dtype) {
  // result type is favored over dtype; check that they match if provided (NumPy doesn't check)
  AT_CHECK(!dtype.has_value() || (result.type().scalarType() == dtype.value()),
            "provided dtype must match dtype of result in cumprod.  Got %s and %s.",
            at::toString(result.type().scalarType()), at::toString(dtype.value()));
  return at::_cumprod_out(result, self.toType(result.type().scalarType()), dim);
}

Tensor& cumprod_out(Tensor& result, const Tensor& self, int64_t dim, ScalarType dtype) {
  return at::native::cumprod_out(result, self, dim, optional<ScalarType>(dtype));
}

Tensor& cumprod_out(Tensor& result, const Tensor& self, int64_t dim) {
  return at::native::cumprod_out(result, self, dim, nullopt);
}

// ALL REDUCE #################################################################

static inline Tensor sum(const Tensor &self, optional<ScalarType> dtype) {
  return at::_sum(integer_upcast(self, dtype));
}

Tensor sum(const Tensor &self, ScalarType dtype) {
  return at::native::sum(self, optional<ScalarType>(dtype));
}

Tensor sum(const Tensor &self) {
  return at::native::sum(self, nullopt);
}

Tensor _sum_cpu(const Tensor& self) {
  if (self.is_contiguous()) {
    Tensor result = self.type().tensor({});
    sum_kernel(result, self, at::nullopt);
    return result;
  }
  return self._sumall();
}

static inline Tensor prod(const Tensor &self, optional<ScalarType> dtype) {
  return at::_prod(integer_upcast(self, dtype));
}

Tensor prod(const Tensor &self, ScalarType dtype) {
  return at::native::prod(self, optional<ScalarType>(dtype));
}

Tensor prod(const Tensor &self) {
  return at::native::prod(self, nullopt);
}

Tensor _prod_cpu(const Tensor &self) {
  if (self.is_contiguous()) {
    Tensor result = self.type().tensor({});
    prod_kernel(result, self, at::nullopt);
    return result;
  }
  return self._prodall();
}

// \ALL REDUCE ################################################################

// DIM REDUCE #################################################################

static bool _dimreduce_return_trivial(Tensor &result, const Tensor &self,
                                      int64_t ident) {
  if (self.numel() == 1 && self.ndimension() == 0) {
    result.resize_({});
    result.fill_(self);
    return true;
  }
  // Return identity
  if (self.numel() == 0 && self.ndimension() == 1) {
    result.resize_({0});
    result.fill_(ident);
    return true;
  }
  return false;
}

static Tensor &_dimreduce_setup(Tensor &result, const Tensor &self,
                                int64_t dim) {
  IntList self_sizes = self.sizes();
  std::vector<int64_t> result_sizes;
  result_sizes.insert(result_sizes.end(), self_sizes.begin(), self_sizes.end());
  result_sizes[dim] = 1;
  result.resize_(result_sizes);
  return result;
}

static inline Tensor &sum_out(Tensor &result, const Tensor &self, IntList dim,
                 bool keepdim, optional<ScalarType> dtype) {
  // result type is favored over dtype; check that they match if provided (NumPy doesn't check)
  AT_CHECK(!dtype.has_value() || (result.type().scalarType() == dtype.value()),
            "provided dtype must match dtype of result in sum.  Got %s and %s.",
            at::toString(result.type().scalarType()), at::toString(dtype.value()));
  return at::_sum_out(result, self.toType(result.type().scalarType()), dim, keepdim);
}

Tensor& sum_out(Tensor& result, const Tensor& self, IntList dim, bool keepdim, ScalarType dtype) {
  return at::native::sum_out(result, self, dim, keepdim, at::optional<ScalarType>(dtype));
}
Tensor& sum_out(Tensor& result, const Tensor& self, IntList dim, bool keepdim) {
  return at::native::sum_out(result, self, dim, keepdim, nullopt);
}

Tensor& sum_out(Tensor& result, const Tensor& self, IntList dim, ScalarType dtype) {
  return at::native::sum_out(result, self, dim, false, dtype);
}

Tensor &_sum_out_cpu(Tensor &result, const Tensor &self, int64_t dim_,
                     bool keepdim) {
  int64_t dim = maybe_wrap_dim(dim_, self.dim());
  if (_dimreduce_return_trivial(result, self, 0))
    return result;
  if (self.is_contiguous() && result.is_contiguous()) {
    _dimreduce_setup(result, self, dim);
    sum_kernel(result, self, dim);
    if (!keepdim) result.squeeze_(dim);
    return result;
  }
  return at::_th_sum_out(result, self, dim, keepdim);
}

static inline Tensor &prod_out(Tensor &result, const Tensor &self, int64_t dim,
                 bool keepdim, optional<ScalarType> dtype) {
  // result type is favored over dtype; check that they match if provided (NumPy doesn't check)
  AT_CHECK(!dtype.has_value() || (result.type().scalarType() == dtype.value()),
            "provided dtype must match dtype of result in prod.  Got %s and %s.",
            at::toString(result.type().scalarType()), at::toString(dtype.value()));
  return at::_prod_out(result, self.toType(result.type().scalarType()), dim, keepdim);
}

Tensor& prod_out(Tensor& result, const Tensor& self, int64_t dim, bool keepdim, ScalarType dtype) {
  return at::native::prod_out(result, self, dim, keepdim, at::optional<ScalarType>(dtype));
}
Tensor& prod_out(Tensor& result, const Tensor& self, int64_t dim, bool keepdim) {
  return at::native::prod_out(result, self, dim, keepdim, nullopt);
}

Tensor& prod_out(Tensor& result, const Tensor& self, int64_t dim, ScalarType dtype) {
  return at::native::prod_out(result, self, dim, false, dtype);
}

Tensor &_prod_out_cpu(Tensor &result, const Tensor &self, int64_t dim_,
                      bool keepdim) {
  int64_t dim = maybe_wrap_dim(dim_, self.dim());
  if (_dimreduce_return_trivial(result, self, 1))
    return result;
  if (self.is_contiguous() && result.is_contiguous()) {
    _dimreduce_setup(result, self, dim);
    prod_kernel(result, self, dim);
    if (!keepdim) result.squeeze_(dim);
    return result;
  }
  return at::_th_prod_out(result, self, dim, keepdim);
}

static inline Tensor sum(const Tensor &self, IntList dim_, bool keepdim, optional<ScalarType> dtype) {
  return at::_sum(integer_upcast(self, dtype), dim_, keepdim);
}

Tensor sum(const Tensor& self, IntList dim, bool keepdim, ScalarType dtype) {
  return at::native::sum(self, dim, keepdim, at::optional<ScalarType>(dtype));
}

Tensor sum(const Tensor& self, IntList dim, bool keepdim) {
  return at::native::sum(self, dim, keepdim, nullopt);
}

Tensor sum(const Tensor& self, IntList dim, ScalarType dtype) {
  return at::native::sum(self, dim, false, dtype);
}

Tensor _sum(const Tensor &self, int64_t dim_, bool keepdim) {
  int64_t dim = maybe_wrap_dim(dim_, self.dim());
  Tensor result = self.type().tensor();
  return at::_sum_out(result, self, dim, keepdim);
}

static inline Tensor prod(const Tensor &self, int64_t dim_, bool keepdim, optional<ScalarType> dtype) {
  return at::_prod(integer_upcast(self, dtype), dim_, keepdim);
}

Tensor prod(const Tensor& self, int64_t dim, bool keepdim, ScalarType dtype) {
  return at::native::prod(self, dim, keepdim, at::optional<ScalarType>(dtype));
}

Tensor prod(const Tensor& self, int64_t dim, bool keepdim) {
  return at::native::prod(self, dim, keepdim, nullopt);
}

Tensor prod(const Tensor& self, int64_t dim, ScalarType dtype) {
  return at::native::prod(self, dim, false, dtype);
}

Tensor _prod(const Tensor &self, int64_t dim_, bool keepdim) {
  int64_t dim = maybe_wrap_dim(dim_, self.dim());
  Tensor result = self.type().tensor();
  return at::_prod_out(result, self, dim, keepdim);
}

Tensor& logsumexp_out(Tensor& result, const Tensor &self, int64_t dim_, bool keepdim) {
  int64_t dim = maybe_wrap_dim(dim_, self.dim());
  auto maxes = at::max_values(self, dim, true);
  result = at::where((maxes == INFINITY).__or__(maxes == -INFINITY),
		     maxes,
		     maxes + at::log(at::sum(at::exp(self - maxes), dim, true)));
  if (! keepdim)
    result.squeeze_(dim);
  return result;
}

Tensor logsumexp(const Tensor &self, int64_t dim_, bool keepdim) {
  int64_t dim = maybe_wrap_dim(dim_, self.dim());
  Tensor result = self.type().tensor();
  return at::native::logsumexp_out(result, self, dim, keepdim);
}

// \DIM REDUCE ################################################################

// MULTI DIM REDUCE ###########################################################

template <Tensor (reduce_1)(const Tensor &, int64_t, bool)>
inline Tensor reduce_multi_associative(const Tensor &self, IntList dims_, bool keepdim) {
  if (dims_.size() == 1) {
    return reduce_1(self, dims_[0], keepdim);
  }
  if (dims_.size() == 0) {
    return self;
  }
  int64_t ndims = self.dim();
  auto reduce_dims = dim_list_to_bitset(dims_, ndims);
  Tensor result = self;
  for (int64_t dim = ndims-1; dim >= 0; dim--) {
    if (reduce_dims[dim])
      result = reduce_1(result, dim, keepdim);
  }
  return result;
}

template <Tensor (reduce_1)(const Tensor &, int64_t, bool),
	  Tensor& (reduce_1_out)(Tensor& result, const Tensor &, int64_t, bool)>
inline Tensor& reduce_multi_associative_out(Tensor &result, const Tensor &self, IntList dims_, bool keepdim) {
  if (dims_.size() == 1) {
    return reduce_1_out(result, self, dims_[0], keepdim);
  }
  int64_t ndims = self.dim();
  auto reduce_dims = dim_list_to_bitset(dims_, ndims);
  Tensor t = self;
  int64_t last_reduction = dims_.size()-1;
  int64_t num_reduction = 0;
  for (int64_t dim = ndims-1; dim >= 0; dim--) {
    if (reduce_dims[dim]) {
      if (num_reduction < last_reduction) {
	t = reduce_1(t, dim, keepdim);
      } else {
	reduce_1_out(result, t, dim, keepdim);
      }
      num_reduction++;
    }
  }
  return result;
}


Tensor& _sum_out(Tensor &result, const Tensor &self, int64_t dim, bool keepdim) {
  if (self.is_cuda()) {
    return at::_sum_cuda_out(result, self, dim, keepdim);
  }
  else {
    return _sum_out_cpu(result, self, dim, keepdim);
  }
}

Tensor _sum(const Tensor &self, IntList dims, bool keepdim) {
  return reduce_multi_associative<_sum>(self, dims, keepdim);
}

Tensor& _sum_out(Tensor &result, const Tensor &self, IntList dims, bool keepdim)
{
  return reduce_multi_associative_out<_sum, _sum_out>(result, self, dims, keepdim);
}

}} // namespace at::native
