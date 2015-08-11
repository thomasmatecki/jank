#pragma once

#include <sstream>

#include <jank/parse/expect/type.hpp>
#include <jank/translate/cell/cell.hpp>
#include <jank/translate/environment/scope.hpp>
#include <jank/translate/function/argument/resolve_type.hpp>
#include <jank/translate/expect/error/type/overload.hpp>

namespace jank
{
  namespace translate
  {
    namespace function
    {
      namespace detail
      {
        template <typename Def>
        struct call
        { using type = cell::function_call; };
        template <>
        struct call<cell::native_function_definition>
        { using type = cell::native_function_call; };

        /* Handles native and non-native functions. */
        template <typename Def>
        std::experimental::optional<typename detail::call<Def>::type>
        match_overload
        (
          parse::cell::list const &list,
          std::shared_ptr<environment::scope> const &scope,
          std::vector<environment::scope::result<Def>> const &functions
        )
        {
          /* TODO: Argument parsing is happening twice: once for non-native, once
           * for native. We should just do it once. */
          auto const arguments
          (function::argument::call::parse<cell::cell>(list, scope));

          for(auto const &overload_cell : functions)
          {
            auto const &overload(overload_cell.first.data);

            if(overload.arguments.size() != arguments.size())
            { continue; }

            if
            (
              std::equal
              (
                overload.arguments.begin(), overload.arguments.end(),
                arguments.begin(),
                [&](auto const &lhs, auto const &rhs)
                {
                  return
                  (
                    lhs.type.definition ==
                    function::argument::resolve_type(rhs.cell, scope).data
                  );
                }
              )
            )
            { return { { { overload, arguments, scope } } }; }
          }
          return {};
        }
      }

      template <typename Native, typename Non_Native, typename Callback>
      void match_overload
      (
        parse::cell::list const &list,
        std::shared_ptr<environment::scope> const &scope,
        Native const &native,
        Non_Native const &non_native,
        Callback const &callback
      )
      {
        auto const match
        (
          [&](auto const &opt)
          {
            if(!opt)
            { return false; }

            auto const matched_opt(detail::match_overload(list, scope, opt.value()));
            if(matched_opt)
            { callback(matched_opt.value()); }
            return static_cast<bool>(matched_opt);
          }
        );

        if(!match(non_native) && !match(native))
        {
          /* No matching overload found. */
          auto const arguments
          (function::argument::call::parse<cell::cell>(list, scope));

          std::stringstream ss;
          ss << "no matching function: "
             << parse::expect::type<parse::cell::type::ident>(list.data[0]).data
             << " with arguments: ";

          for(auto const &arg : arguments)
          {
            ss << arg.name << " : "
               << function::argument::resolve_type(arg.cell, scope).data.name
               << " ";
          }
          throw expect::error::type::overload
          { ss.str() };
        }
      }
    }
  }
}
