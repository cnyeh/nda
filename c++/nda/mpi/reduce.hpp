#pragma once
#include <mpi/mpi.hpp>

namespace nda::lazy_mpi {

  // Models ArrayInitializer concept
  template <typename ValueType, int Rank, uint64_t StrideOrder>
  struct reduce {

    using view_t     = array_contiguous_view<ValueType const, Rank, StrideOrder>;
    using value_type = ValueType;

    view_t source;       // view of the array to reduce
    mpi::communicator c; // mpi comm
    const int root;      //
    const bool all;
    const MPI_Op op;

    /// compute the shape of the target array
    [[nodiscard]] auto shape() const { return source.shape(); }

    /// Delayed reduction operation
    void invoke(view_t target_view) const {
      // we force the caller to build a view_t. If not possible, e.g. stride orders mismatch, it will not compile

      // some checks.
      bool in_place = (target_view.data_start() == source.data_start());
      auto sha      = shape(); 
      if (in_place) {
        if (source.size() != target_view.size())
          NDA_RUNTIME_ERROR << "mpi reduce of array : same pointer to data start, but different number of elements !";
      } else { // check no overlap
        if ((c.rank() == root) || all) resize_or_check_if_view(target_view, sha);
        if (std::abs(target_view.data_start() - source.data_start()) < source.size())
          NDA_RUNTIME_ERROR << "mpi reduce of array : overlapping arrays !";
      }

      void *v_p       = (void *)target_view.data_start();
      void *rhs_p     = (void *)source.data_start();
      auto rhs_n_elem = source.size();
      auto D          = mpi::mpi_type<value_type>::get();

      if (!all) {
        if (in_place)
          MPI_Reduce((c.rank() == root ? MPI_IN_PLACE : rhs_p), rhs_p, rhs_n_elem, D, op, root, c.get());
        else
          MPI_Reduce(rhs_p, v_p, rhs_n_elem, D, op, root, c.get());
      } else {
        if (in_place)
          MPI_Allreduce(MPI_IN_PLACE, rhs_p, rhs_n_elem, D, op, c.get());
        else
          MPI_Allreduce(rhs_p, v_p, rhs_n_elem, D, op, c.get());
      }
    }
  };

  //----------------------------  mark the class for C++17 concept workaround

#if not __cplusplus > 201703L

  template <typename V>
  inline constexpr bool is_array_initializer_v<reduce<V>> = true;

#endif

} // namespace nda::lazy_mpi

//----------------------------

namespace nda {

  /**
   * Reduction of the array
   *
   * \tparam A basic_array or basic_array_view, with contiguous data only
   * \param a
   * \param c The MPI communicator
   * \param root Root node of the reduction
   * \param all all_reduce iif true
   * \param op The MPI reduction operation to apply to the elements 
   *
   * NB : A::value_type must have an MPI reduction (basic type or custom type, cf mpi library)
   *
   */
  template <typename A>
  AUTO(ArrayInitializer)
  mpi_reduce(A &a, mpi::communicator c = {}, int root = 0, bool all = false, MPI_Op op = MPI_SUM) //
     REQUIRES(is_regular_or_view_v<A>) {
    static_assert(has_layout_contiguous<A>, "Non contigous view in target_view.data_start() are not implemented");
    static_assert(mpi::has_mpi_type<typename A::value_type>, "Reduction of non MPI types is not implemented");
    return lazy_mpi::reduce<typename A::value_type, A::rank, A::layout_t::stride_order_encoded>{a(), c, root, all, op};
  }

} // namespace nda