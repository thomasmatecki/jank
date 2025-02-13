#pragma once

#include <functional>

#include <jank/read/parse.hpp>
#include <jank/runtime/context.hpp>
#include <jank/runtime/object.hpp>
#include <jank/runtime/obj/symbol.hpp>
#include <jank/runtime/obj/list.hpp>
#include <jank/runtime/obj/vector.hpp>
#include <jank/analyze/frame.hpp>
#include <jank/analyze/expression.hpp>
#include <jank/option.hpp>

namespace jank::analyze
{
  struct lifted_var
  {
    runtime::obj::symbol local_name;
    runtime::obj::symbol_ptr var_name;
  };

  struct lifted_constant
  {
    runtime::obj::symbol local_name;
    runtime::object_ptr data;
  };

  struct context
  {
    context() = delete;
    context(runtime::context &rt_ctx);

    runtime::obj::symbol_ptr lift_var(runtime::obj::symbol_ptr const &);
    option<std::reference_wrapper<lifted_var>> find_lifted_var(runtime::obj::symbol_ptr const &);
    void lift_constant(runtime::object_ptr const &);
    option<std::reference_wrapper<lifted_constant>> find_lifted_constant(runtime::object_ptr const &);

    static runtime::obj::symbol unique_name();
    static runtime::obj::symbol unique_name(std::string const &prefix);

    runtime::context &rt_ctx;
    std::unordered_map<runtime::obj::symbol_ptr, lifted_var> lifted_vars;
    std::unordered_map<runtime::object_ptr, lifted_constant> lifted_constants;
  };

  using error = read::error;

  struct processor
  {
    using expression_result = result<option<expression>, error>;

    struct iterator
    {
      using iterator_category = std::input_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = expression_result;
      using pointer = value_type*;
      using reference = value_type&;

      value_type operator *() const;
      pointer operator ->();
      iterator& operator ++();
      bool operator !=(iterator const &rhs) const;
      bool operator ==(iterator const &rhs) const;

      /* TODO: Get rid of option. */
      value_type latest;
      context &ctx;
      processor &p;
      bool is_end{};
    };

    processor() = delete;
    processor(runtime::context &rt_ctx, read::parse::processor::iterator const &b, read::parse::processor::iterator const &e);
    processor(processor const &) = default;
    processor(processor &&) = default;

    iterator begin(context &ctx);
    iterator end(context &ctx);

    expression_result next(context &ctx);
    expression_result analyze(runtime::object_ptr const &, context &);
    expression_result analyze(runtime::object_ptr const &, frame<expression> &, context &);
    expression_result analyze_call(runtime::obj::list_ptr const &, frame<expression> &, context &);
    expression_result analyze_def(runtime::obj::list_ptr const &, frame<expression> &, context &);
    expression_result analyze_symbol(runtime::obj::symbol_ptr const &, frame<expression> &, context &);
    expression_result analyze_fn(runtime::obj::list_ptr const &, frame<expression> &, context &);
    expression_result analyze_let(runtime::obj::list_ptr const &, frame<expression> &, context &);
    expression_result analyze_if(runtime::obj::list_ptr const &, frame<expression> &, context &);
    expression_result analyze_quote(runtime::obj::list_ptr const &, frame<expression> &, context &);
    expression_result analyze_primitive_literal(runtime::object_ptr const &, frame<expression> &, context &);
    expression_result analyze_vector(runtime::obj::vector_ptr const &, frame<expression> &, context &);

    using special_function_type = std::function<expression_result (runtime::obj::list_ptr const &, frame<expression> &, context &)>;
    std::unordered_map<runtime::obj::symbol_ptr, special_function_type> specials;
    runtime::context &rt_ctx;
    frame<expression> root_frame;
    read::parse::processor::iterator parse_current, parse_end;
    size_t total_forms{};
  };
}
