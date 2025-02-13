#include <doctest/doctest.h>

#include <jank/read/lex.hpp>
#include <jank/read/parse.hpp>
#include <jank/runtime/obj/number.hpp>
#include <jank/analyze/processor.hpp>

namespace jank::analyze::expr
{
  TEST_CASE("Empty")
  {
    read::lex::processor l_prc{ "(let* [])" };
    read::parse::processor p_prc{ l_prc.begin(), l_prc.end() };
    runtime::context rt_ctx;
    context an_ctx{ rt_ctx };
    processor an_prc{ rt_ctx, p_prc.begin(), p_prc.end() };

    auto const expr(an_prc.next(an_ctx).expect_ok().unwrap());
    auto const *let_expr(boost::get<let<expression>>(&expr.data));
    CHECK(let_expr != nullptr);
    CHECK(let_expr->local_frame.locals.empty());
    CHECK(let_expr->body.body.empty());
  }

  TEST_CASE("Bindings")
  {
    runtime::context rt_ctx;
    context an_ctx{ rt_ctx };

    SUBCASE("Literal")
    {
      read::lex::processor l_prc{ "(let* [a 1] a)" };
      read::parse::processor p_prc{ l_prc.begin(), l_prc.end() };
      processor an_prc{ rt_ctx, p_prc.begin(), p_prc.end() };

      auto const expr(an_prc.next(an_ctx).expect_ok().unwrap());
      auto const *let_expr(boost::get<let<expression>>(&expr.data));
      CHECK(let_expr != nullptr);
      CHECK(let_expr->local_frame.locals.size() == 1);
      CHECK(let_expr->body.body.size() == 1);
    }

    SUBCASE("Reference previous")
    {
      read::lex::processor l_prc{ "(let* [a 1 b a] b)" };
      read::parse::processor p_prc{ l_prc.begin(), l_prc.end() };
      processor an_prc{ rt_ctx, p_prc.begin(), p_prc.end() };

      auto const expr(an_prc.next(an_ctx).expect_ok().unwrap());
      auto const *let_expr(boost::get<let<expression>>(&expr.data));
      CHECK(let_expr != nullptr);
      CHECK(let_expr->local_frame.locals.size() == 2);
      CHECK(let_expr->body.body.size() == 1);
    }

    SUBCASE("Shadow")
    {
      read::lex::processor l_prc{ "(let* [a 1 a 2] a)" };
      read::parse::processor p_prc{ l_prc.begin(), l_prc.end() };
      processor an_prc{ rt_ctx, p_prc.begin(), p_prc.end() };

      auto const expr(an_prc.next(an_ctx).expect_ok().unwrap());
      auto const *let_expr(boost::get<let<expression>>(&expr.data));
      CHECK(let_expr != nullptr);
      CHECK(let_expr->local_frame.locals.size() == 1);
      CHECK(let_expr->body.body.size() == 1);
    }
  }

  TEST_CASE("Misuse")
  {
    runtime::context rt_ctx;
    context an_ctx{ rt_ctx };

    SUBCASE("Non-symbol binding")
    {
      read::lex::processor l_prc{ "(let* [1 1])" };
      read::parse::processor p_prc{ l_prc.begin(), l_prc.end() };
      processor an_prc{ rt_ctx, p_prc.begin(), p_prc.end() };
      CHECK(an_prc.next(an_ctx).is_err());
    }

    SUBCASE("Qualified symbol binding")
    {
      read::lex::processor l_prc{ "(let* [foo.bar/spam 1])" };
      read::parse::processor p_prc{ l_prc.begin(), l_prc.end() };
      processor an_prc{ rt_ctx, p_prc.begin(), p_prc.end() };
      CHECK(an_prc.next(an_ctx).is_err());
    }

    SUBCASE("Self-referenced binding")
    {
      read::lex::processor l_prc{ "(let* [a a])" };
      read::parse::processor p_prc{ l_prc.begin(), l_prc.end() };
      processor an_prc{ rt_ctx, p_prc.begin(), p_prc.end() };
      CHECK_THROWS(an_prc.next(an_ctx));
    }
  }
}
