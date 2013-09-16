/*
Copyright (c) 2013 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include <algorithm>
#include <vector>
#include <utility>
#include "util/test.h"
#include "kernel/metavar.h"
#include "kernel/instantiate.h"
#include "kernel/abstract.h"
#include "kernel/free_vars.h"
#include "library/printer.h"
using namespace lean;

class unification_problems_dbg : public unification_problems {
    std::vector<std::pair<expr, expr>> m_eqs;
    std::vector<std::pair<expr, expr>> m_type_of_eqs;
public:
    unification_problems_dbg() {}
    virtual ~unification_problems_dbg() {}
    virtual void add_eq(context const &, expr const & lhs, expr const & rhs) { m_eqs.push_back(mk_pair(lhs, rhs)); }
    virtual void add_type_of_eq(context const &, expr const & n, expr const & t) { m_type_of_eqs.push_back(mk_pair(n, t)); }
    std::vector<std::pair<expr, expr>> const & eqs() const { return m_eqs; }
    std::vector<std::pair<expr, expr>> const & type_of_eqs() const { return m_type_of_eqs; }
};

static void tst1() {
    unification_problems_dbg u;
    metavar_env          menv;
    expr m1 = menv.mk_metavar();
    lean_assert(!menv.is_assigned(m1));
    lean_assert(menv.contains(m1));
    lean_assert(!menv.contains(2));
    expr t1 = menv.get_type(m1, u);
    lean_assert(is_metavar(t1));
    lean_assert(menv.contains(t1));
    lean_assert(is_eqp(menv.get_type(m1, u), t1));
    lean_assert(is_eqp(menv.get_type(m1, u), t1));
    lean_assert(!menv.is_assigned(m1));
    expr m2 = menv.mk_metavar();
    lean_assert(!menv.is_assigned(m1));
    lean_assert(menv.contains(m1));
    expr t2 = menv.get_type(m2, u);
    lean_assert(is_metavar(m2));
    lean_assert(menv.contains(m2));
    lean_assert(!is_eqp(t1, t2));
    lean_assert(t1 != t2);
    lean_assert(u.eqs().empty());
    lean_assert(u.type_of_eqs().size() == 2);
    for (auto p : u.type_of_eqs()) {
        std::cout << "typeof(" << p.first << ") == " << p.second << "\n";
    }
    expr f = Const("f");
    expr a = Const("a");
    menv.assign(m1, f(a));
    lean_assert(menv.is_assigned(m1));
    lean_assert(!menv.is_assigned(m2));
    lean_assert(menv.get_subst(m1) == f(a));
}

static void tst2() {
    metavar_env menv;
    expr f = Const("f");
    expr g = Const("g");
    expr h = Const("h");
    expr a = Const("a");
    expr m1 = menv.mk_metavar();
    expr m2 = menv.mk_metavar();
    // move m1 to a different context, and store new metavariable + context in m11
    expr m11 = add_lower(add_subst(m1, 0, f(a, m2)), 1, 1);
    std::cout << m11 << "\n";
    menv.assign(m1, f(Var(0)));
    std::cout << instantiate_metavars(m11, menv) << "\n";
    lean_assert(instantiate_metavars(m11, menv) == f(f(a, add_lower(m2, 1, 1))));
    menv.assign(m2, g(a, Var(1)));
    std::cout << instantiate_metavars(h(m11), menv) << "\n";
    lean_assert(instantiate_metavars(h(m11), menv) == h(f(f(a, g(a, Var(0))))));
}

static void tst3() {
    metavar_env menv;
    expr f = Const("f");
    expr g = Const("g");
    expr h = Const("h");
    expr a = Const("a");
    expr x = Const("x");
    expr T = Const("T");
    expr m1 = menv.mk_metavar();
    expr F = Fun({x, T}, f(m1, x));
    menv.assign(m1, h(Var(0), Var(2)));
    std::cout << instantiate(abst_body(F), g(a)) << "\n";
    std::cout << instantiate_metavars(instantiate(abst_body(F), g(a)), menv) << "\n";
    lean_assert(instantiate_metavars(instantiate(abst_body(F), g(a)), menv) == f(h(g(a), Var(1)), g(a)));
    std::cout << instantiate(instantiate_metavars(abst_body(F), menv), g(a)) << "\n";
    lean_assert(instantiate(instantiate_metavars(abst_body(F), menv), g(a)) ==
                instantiate_metavars(instantiate(abst_body(F), g(a)), menv));
}

static void tst4() {
    metavar_env menv;
    expr f = Const("f");
    expr g = Const("g");
    expr h = Const("h");
    expr a = Const("a");
    expr m1 = menv.mk_metavar();
    expr F = f(m1, Var(2));
    menv.assign(m1, h(Var(1)));
    std::cout << instantiate(F, {g(Var(0)), h(a)}) << "\n";
    std::cout << instantiate_metavars(instantiate(F, {g(Var(0)), h(a)}), menv) << "\n";
}

static void tst5() {
    metavar_env menv;
    expr m1 = menv.mk_metavar();
    expr f  = Const("f");
    expr m11 = add_lower(m1, 2, 1);
    std::cout << add_subst(add_lower(m1, 2, 1), 1, f(Var(0))) << "\n";
    std::cout << add_subst(add_lower(m1, 2, 1), 1, f(Var(3))) << "\n";
    std::cout << add_subst(add_lower(m1, 2, 1), 3, f(Var(0))) << "\n";
    std::cout << add_subst(add_lower(add_lower(m1, 2, 1), 3, 1), 3, f(Var(0))) << "\n";
    std::cout << add_lower(add_lift(m1, 1, 1), 1, 1) << "\n";
    std::cout << add_lower(add_lift(m1, 1, 1), 2, 1) << "\n";
    std::cout << add_lower(add_lift(m1, 1, 1), 2, 2) << "\n";
    std::cout << add_lower(add_lift(m1, 1, 3), 2, 2) << "\n";
    std::cout << add_subst(add_lift(m1, 1, 1), 0, f(Var(0))) << "\n";
    std::cout << add_subst(add_lift(m1, 1, 1), 1, f(Var(0))) << "\n";
    lean_assert(add_subst(add_lower(m1, 2, 1), 1, f(Var(0))) ==
                add_lower(add_subst(m1, 1, f(Var(0))), 2, 1));
    lean_assert(add_subst(add_lower(m1, 2, 1), 1, f(Var(3))) ==
                add_lower(add_subst(m1, 1, f(Var(4))), 2, 1));
    lean_assert(add_subst(add_lower(m1, 2, 1), 2, f(Var(0))) ==
                add_lower(add_subst(m1, 3, f(Var(0))), 2, 1));
    lean_assert(add_subst(add_lower(add_lower(m1, 2, 1), 3, 1), 3, f(Var(0))) ==
                add_lower(add_lower(add_subst(m1, 5, f(Var(0))), 2, 1), 3, 1));
    lean_assert(add_lower(add_lift(m1, 1, 1), 2, 1) == m1);
    lean_assert(add_lower(add_lift(m1, 1, 3), 2, 2) == add_lift(m1, 1, 1));
    lean_assert(add_subst(add_lift(m1, 1, 1), 0, f(Var(0))) ==
                add_lift(add_subst(m1, 0, f(Var(0))), 1, 1));
    lean_assert(add_subst(add_lift(m1, 1, 1), 1, f(Var(0))) ==
                add_lift(m1, 1, 1));
}

static void tst6() {
    expr N  = Const("N");
    expr f  = Const("f");
    expr x  = Const("x");
    expr y  = Const("y");
    expr a  = Const("a");
    expr g  = Const("g");
    expr h  = Const("h");
    metavar_env menv;
    expr m1 = menv.mk_metavar();
    expr m2 = menv.mk_metavar();
    expr t = f(Var(0), Fun({x, N}, f(Var(1), x, Fun({y, N}, f(Var(2), x, y)))));
    expr r = instantiate(t, g(m1, m2));
    std::cout << r << std::endl;
    menv.assign(1, Var(2));
    r = instantiate_metavars(r, menv);
    std::cout << r << std::endl;
    menv.assign(0, h(Var(3)));
    r = instantiate_metavars(r, menv);
    std::cout << r << std::endl;
    lean_assert(r == f(g(h(Var(3)), Var(2)), Fun({x, N}, f(g(h(Var(4)), Var(3)), x, Fun({y, N}, f(g(h(Var(5)), Var(4)), x, y))))));
}

static void tst7() {
    expr f  = Const("f");
    expr g  = Const("g");
    expr a  = Const("a");
    metavar_env menv;
    expr m1 = menv.mk_metavar();
    expr t  = f(m1, Var(0));
    expr r = instantiate(t, a);
    menv.assign(0, g(Var(0)));
    r = instantiate_metavars(r, menv);
    std::cout << r << std::endl;
    lean_assert(r == f(g(a), a));
}

static void tst8() {
    expr f  = Const("f");
    expr g  = Const("g");
    expr a  = Const("a");
    metavar_env menv;
    expr m1 = menv.mk_metavar();
    expr t  = f(m1, Var(0), Var(2));
    expr r = instantiate(t, a);
    menv.assign(0, g(Var(0), Var(1)));
    r = instantiate_metavars(r, menv);
    std::cout << r << std::endl;
    lean_assert(r == f(g(a, Var(0)), a, Var(1)));
}

static void tst9() {
    expr f  = Const("f");
    expr g  = Const("g");
    expr a  = Const("a");
    metavar_env menv;
    expr m1 = menv.mk_metavar();
    expr t  = f(m1, Var(1), Var(2));
    expr r  = lift_free_vars(t, 1, 2);
    std::cout << r << std::endl;
    r = instantiate(r, a);
    std::cout << r << std::endl;
    menv.assign(0, g(Var(0), Var(1)));
    r = instantiate_metavars(r, menv);
    std::cout << r << std::endl;
    lean_assert(r == f(g(a, Var(2)), Var(2), Var(3)));
}

static void tst10() {
    expr N  = Const("N");
    expr f  = Const("f");
    expr x  = Const("x");
    expr y  = Const("y");
    expr a  = Const("a");
    expr g  = Const("g");
    expr h  = Const("h");
    metavar_env menv;
    expr m1 = menv.mk_metavar();
    expr m2 = menv.mk_metavar();
    expr t = f(Var(0), Fun({x, N}, f(Var(1), Var(2), x, Fun({y, N}, f(Var(2), x, y)))));
    expr r = instantiate(t, g(m1));
    std::cout << r << std::endl;
    r = instantiate(r, h(m2));
    std::cout << r << std::endl;
    menv.assign(0, f(Var(0)));
    menv.assign(1, Var(2));
    r = instantiate_metavars(r, menv);
    std::cout << r << std::endl;
    lean_assert(r == f(g(f(h(Var(2)))), Fun({x, N}, f(g(f(h(Var(3)))), h(Var(3)), x, Fun({y, N}, f(g(f(h(Var(4)))), x, y))))));
}

int main() {
    tst1();
    tst2();
    tst3();
    tst4();
    tst5();
    tst6();
    tst7();
    tst8();
    tst9();
    tst10();
    return has_violations() ? 1 : 0;
}
