// Copyright (c) 2019-2021 Simons Foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Authors: Olivier Parcollet, Nils Wentzell

#pragma once
#include <complex>
#include <string_view>
#include "nda/exceptions.hpp"
#include "nda/traits.hpp"
#include "nda/declarations.hpp"
#include "nda/mem/address_space.hpp"

#if defined(NDA_HAVE_TBLIS)
#include "interface/tblis_interface.hpp"
#endif

#if defined(NDA_HAVE_CUTENSOR)
#include "interface/cutensor_interface.hpp"
#endif

namespace nda::tensor {

  /**
   * Compute b(...) <- alpha a(...) + beta * b(...) using one of the tensor ops backend. 
   *
   * @param b Out parameter. Can be a temporary view (hence the &&).
   * @param ... Tensor index in einstein notation, provided as a string_view. 
   *
   * @Precondition :
   *       * Tensor ranks must match the size of the provided index list (string_view object).  
   */
  template <Array X, MemoryArray B>
  requires((MemoryArray<X> or nda::blas::is_conj_array_expr<X>) and                        
           have_same_value_type_v<X, B> and is_blas_lapack_v<get_value_t<X>>) 
  void add(get_value_t<X> alpha, X const &x, std::string_view const indxX, 
	   get_value_t<X> beta , B &&b, std::string_view const indxY) {

    using nda::blas::is_conj_array_expr;
    using value_t = get_value_t<X>;
    auto to_mat = []<typename Z>(Z const &z) -> auto & {
      if constexpr (is_conj_array_expr<Z>)
        return std::get<0>(z.a);
      else
        return z;
    };
    auto &a = to_mat(x);

    static constexpr bool conj_A = is_conj_array_expr<X>;

    using A = decltype(a);
    static_assert(mem::have_compatible_addr_space_v<A, B>, "Matrices must have compatible memory address space");

    if( get_rank<A> != indxX.size() ) NDA_RUNTIME_ERROR <<"tensor::add: Rank mismatch \n";
    if( get_rank<B> != indxY.size() ) NDA_RUNTIME_ERROR <<"tensor::add: Rank mismatch \n";
    if( get_rank<A> != get_rank<B> ) NDA_RUNTIME_ERROR <<"tensor::add: Rank mismatch \n";

    if constexpr (mem::have_device_compatible_addr_space_v<A,B>) {
#if defined(NDA_HAVE_CUTENSOR)
      op::TENSOR_OP a_op = conj_A ? op::CONJ : op::ID;
      cutensor::cutensor_desc<value_t,get_rank<A>> a_t(a,a_op);
      cutensor::cutensor_desc<value_t,get_rank<B>> b_t(b,op::ID);
      cutensor::elementwise_binary(alpha,a_t,a.data(),indxX.data(),
				   beta ,b_t,b.data(),indxY.data(),
				   b.data(),op::SUM);
#else
      static_assert(always_false<bool>," add on device requires gpu tensor operations backend. ");
#endif
    } else {
#if defined(NDA_HAVE_TBLIS)
      // no conj in tblis yet!
      static_assert(not conj_A, "Error: No conj in tblis yet!");
      nda_tblis::tensor<value_t,get_rank<A>> a_t(a,alpha);
      nda_tblis::tensor<value_t,get_rank<B>> b_t(b,beta);
      ::tblis::tblis_tensor_add(NULL,NULL,&a_t,indxX.data(),&b_t,indxY.data());
#else
      static_assert(always_false<bool>," add on host requires cpu tensor operations backend. ");
#endif
    }
  }

  template <Array X, Array Y, MemoryArray C>
  requires((MemoryArray<X> or nda::blas::is_conj_array_expr<X>) and
           (MemoryArray<Y> or nda::blas::is_conj_array_expr<Y>) and
           have_same_value_type_v<X, Y, C> and is_blas_lapack_v<get_value_t<X>>)
  void add(get_value_t<X> alpha, X const &x, std::string_view const indxX,
           get_value_t<Y> beta , Y const &y, std::string_view const indxY,
	   C &&c, std::string_view const indxC) 
  {
    using nda::blas::is_conj_array_expr;
    using value_t = get_value_t<X>;
    auto to_mat = []<typename Z>(Z const &z) -> auto & {
      if constexpr (is_conj_array_expr<Z>)
        return std::get<0>(z.a);
      else
        return z;
    };
    auto &a = to_mat(x);
    auto &b = to_mat(y);

    static constexpr bool conj_A = is_conj_array_expr<X>;
    static constexpr bool conj_B = is_conj_array_expr<Y>;

    using A = decltype(a);
    using B = decltype(b);
    static_assert(mem::have_compatible_addr_space_v<A, B, C>, "Matrices must have compatible memory address space");

    if( get_rank<A> != indxX.size() ) NDA_RUNTIME_ERROR <<"tensor::add: Rank mismatch \n";
    if( get_rank<B> != indxY.size() ) NDA_RUNTIME_ERROR <<"tensor::add: Rank mismatch \n";
    if( get_rank<C> != indxC.size() ) NDA_RUNTIME_ERROR <<"tensor::add: Rank mismatch \n";
    if( get_rank<A> != get_rank<B> ) NDA_RUNTIME_ERROR <<"tensor::add: Rank mismatch \n";
    if( get_rank<A> != get_rank<C> ) NDA_RUNTIME_ERROR <<"tensor::add: Rank mismatch \n";
    if( indxY != indxC ) NDA_RUNTIME_ERROR <<"tensor::add: indxB != indxC";
    if( b.strides() != c.strides() or b.shape() != c.shape() or b.stride_order() != c.stride_order() ) 
      NDA_RUNTIME_ERROR <<" tensor::add: Tensor's B and C must have identical strides, shapes and stride_orders."; 

    if constexpr (mem::have_device_compatible_addr_space_v<A,B>) {
#if defined(NDA_HAVE_CUTENSOR)
      op::TENSOR_OP a_op = conj_A ? op::CONJ : op::ID;
      op::TENSOR_OP b_op = conj_B ? op::CONJ : op::ID;
      cutensor::cutensor_desc<value_t,get_rank<A>> a_t(a,a_op);
      cutensor::cutensor_desc<value_t,get_rank<B>> b_t(b,b_op);
      cutensor::elementwise_binary(alpha,a_t,a.data(),indxX,
                                   beta ,b_t,b.data(),indxY,
                                   c.data(),op::SUM);
#else
      static_assert(always_false<bool>," add on device requires gpu tensor operations backend. ");
#endif
    } else { // on host
#if defined(NDA_HAVE_TBLIS)
      // no conj in tblis yet!
      static_assert(not conj_A and not conj_B, "Error: No conj in tblis yet!");
      nda_tblis::tensor<value_t,get_rank<A>> a_t(a,alpha);
      nda_tblis::tensor<value_t,get_rank<C>> c_t(c,value_t{1.0});
      // if conditions on B/C being compatible are relaxed, this needs to change!
      c() = beta*b();   
      ::tblis::tblis_tensor_add(NULL,NULL,&a_t,indxX.data(),&c_t,indxC.data());
#else 
      static_assert(always_false<bool>," add on host requires cpu tensor operations backend. ");
#endif 
    }
  }

  template <Array X, MemoryArray B>
  requires((MemoryArray<X> or nda::blas::is_conj_array_expr<X>) and
           have_same_value_type_v<X, B> and is_blas_lapack_v<get_value_t<X>>)
  void add(X const &x, std::string_view const indxX,
           B &&b, std::string_view const indxY) 
  {
    return add(get_value_t<X>{1.0},x,indxX,get_value_t<X>{0.0},std::forward<B>(b),indxY);
  }

  template <Array X, Array Y, MemoryArray C>
  requires((MemoryArray<X> or nda::blas::is_conj_array_expr<X>) and
           (MemoryArray<Y> or nda::blas::is_conj_array_expr<Y>) and
           have_same_value_type_v<X, Y, C> and is_blas_lapack_v<get_value_t<X>>)
  void add(X const &x, std::string_view const indxX,
           Y const &y, std::string_view const indxY,
           C &&c, std::string_view const indxC)
  {
    return add(get_value_t<X>{1.0},x,indxX,get_value_t<Y>{0.0},y,indxY,std::forward<C>(c),indxC);
  }



} // namespace nda::tensor