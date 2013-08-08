/*
  Copyright (c) 2013 Microsoft Corporation. All rights reserved.
  Released under Apache 2.0 license as described in the file LICENSE.

  Author: Soonho Kong
*/
#include "sexpr.h"
#include "format.h"
#include "sexpr_funcs.h"

namespace lean {
static int default_width = 78;
std::ostream & layout(std::ostream & out, sexpr const & s) {
    lean_assert(!is_nil(s));
    switch (format::sexpr_kind(s)) {
    case format::format_kind::NEST:
    case format::format_kind::CHOICE:
    case format::format_kind::COMPOSE:
        lean_unreachable();
        break;

    case format::format_kind::NIL:
        out << "";
        break;

    case format::format_kind::LINE:
        out << std::endl;
        break;

    case format::format_kind::TEXT:
    {
        sexpr const & v = cdr(s);
        if(is_string(v)) {
            out << to_string(v);
        } else {
            out << v;
        }
        break;
    }

    case format::format_kind::COLOR_BEGIN:
    {
        format::format_color c = static_cast<format::format_color>(to_int(cdr(s)));
        out << "\e[" << (31 + c % 7) << "m";
        break;
    }

    case format::format_kind::COLOR_END:
        out << "\e[0m";
        break;
    }
    return out;
}

std::ostream & layout_list(std::ostream & out, sexpr const & s) {
    foreach(s, [&out](sexpr const & s) {
            layout(out, s);
        });
    return out;
}

format compose(format const & f1, format const & f2) {
    return format(format::sexpr_compose({f1.m_value, f2.m_value}));
}
format nest(int i, format const & f) {
    return format(format::sexpr_nest(i, f.m_value));
}
format highlight(format const & f, format::format_color const c) {
    return format(format::sexpr_highlight(f.m_value, c));
}
format line() {
    return format(format::sexpr_line());
}

sexpr format::flatten(sexpr const & s) {
    lean_assert(is_cons(s));
    switch (sexpr_kind(s)) {
    case format_kind::NIL:
        /* flatten NIL = NIL */
        return s;
    case format_kind::NEST:
        /* flatten (NEST i x) = flatten x */
        return flatten(sexpr_nest_s(s));
    case format_kind::COMPOSE:
        /* flatten (s_1 <> ... <> s_n ) = flatten s_1 <> ... <> flatten s_n */
        return sexpr_compose(map(sexpr_compose_list(s),
                                 [](sexpr const & s) {
                                     return flatten(s);
                                 }));
    case format_kind::CHOICE:
        /* flatten (x <|> y) = flatten x */
        return flatten(sexpr_choice_1(s));
    case format_kind::LINE:
        return sexpr_text(sexpr(" "));
    case format_kind::TEXT:
    case format_kind::COLOR_BEGIN:
    case format_kind::COLOR_END:
        return s;
    }
    lean_unreachable();
    return s;
}
format format::flatten(format const & f){
    return format(flatten(f.m_value));
}
format group(format const & f) {
    return choice(format::flatten(f), f);
}
format above(format const & f1, format const & f2) {
    return format{f1, line(), f2};
}
format bracket(std::string const l, format const & x, std::string const r) {
    return group(format{format(l),
                 nest(2, format(line(), x)),
                 line(),
                 format(r)});
}
format paren(format const & x) {
    return bracket("(", x, ")");
}

// wrap = <+/>
// wrap x y = x <> (text " " :<|> line) <> y
format wrap(format const & f1, format const & f2) {
    return format{f1,
                  choice(format(" "), line()),
                  f2};
}

unsigned format::space_upto_line_break_list(sexpr const & r, bool & found) {
    // r : list of (int * format)
    lean_assert(!found);
    if (is_nil(r)) {
        found = true;
        return 0;
    }
    return format::space_upto_line_break(cdr(car(r)), cdr(r), found);
}

unsigned format::space_upto_line_break(sexpr const & s, sexpr const & r, bool & found) {
    // s : format
    // r : list of (int * format)
    lean_assert(!found);
    if(is_nil(s)) {
        found = true;
        return 0;
    }

    switch (sexpr_kind(s)) {
    case format_kind::NIL:
    case format_kind::COLOR_BEGIN:
    case format_kind::COLOR_END:
        return 0 + space_upto_line_break_list(r, found);
    case format_kind::COMPOSE:
    {
        sexpr list  = sexpr_compose_list(s);
        unsigned len = 0;
        while(!is_nil(list) && !found) {
            sexpr const & h = car(list);
            list = cdr(list);
            len += space_upto_line_break(h, nil(), found);
        }
        if (found) {
            return len;
        } else {
            return len + space_upto_line_break_list(r, found);
        }
    }
    case format_kind::NEST:
    {
        sexpr const & x = sexpr_nest_s(s);
        unsigned len = space_upto_line_break(x, nil(), found);
        if (found) {
            return len;
        } else {
            return len + space_upto_line_break_list(r, found);
        }
    }
    case format_kind::TEXT:
        return sexpr_text_length(s) + space_upto_line_break_list(r, found);
    case format_kind::LINE:
        found = true;
        return 0;
    case format_kind::CHOICE:
    {
        sexpr const & x = sexpr_choice_1(s);
        return space_upto_line_break(x, r, found);
    }
    }
    std::cerr << "!!!!!!!!!!!!!!!" << s << std::endl;
    lean_unreachable();
    return 0;
}

sexpr format::be(unsigned w, unsigned k, sexpr const & s, sexpr const & r) {
    /* be w k [] = Nil */
    if(is_nil(s)) {
        if(is_nil(r)) {
            // s == Nil && r == Nil
            return sexpr();
        } else {
            // s == Nil && r != Nil
            return be(w, k, car(r), cdr(r));
        }
    }

    /* s = (i, v) :: z, where h has the type int x format */
    sexpr const & h = car(s);
    sexpr const & z = cdr(s);
    int i = to_int(car(h));
    sexpr const & v = cdr(h);

    switch (sexpr_kind(v)) {
    case format_kind::NIL:
        return be(w, k, z, r);
    case format_kind::COLOR_BEGIN:
    case format_kind::COLOR_END:
        return sexpr(v, be(w, k, z, r));
    case format_kind::COMPOSE:
    {
        sexpr const & list  = sexpr_compose_list(v);
        sexpr const & list_ = map(list,
                                  [i](sexpr const & s) {
                                      return sexpr(i, s);
                                      });
        return be(w, k, list_, sexpr(z, r));
    }
    case format_kind::NEST:
    {
        int j = sexpr_nest_i(v);
        sexpr const & x = sexpr_nest_s(v);
        return be(w, k, sexpr(sexpr(i + j, x), z), r);
    }
    case format_kind::TEXT:
        return sexpr(v, be(w, k + sexpr_text_length(v), z, r));
    case format_kind::LINE:
        return sexpr(v, sexpr(sexpr_text(std::string(i, ' ')), be(w, i, z, r)));
    case format_kind::CHOICE:
    {
        sexpr const & x = sexpr_choice_1(v);
        sexpr const & y = sexpr_choice_2(v);;
        bool found = false;
        int d = static_cast<int>(w) - static_cast<int>(k) - space_upto_line_break(x, r, found);
        if(d >= 0) {
            sexpr const & s1 = be(w, k, sexpr(sexpr(i, x), z), r);
            return s1;
        }
        sexpr const & s2 = be(w, k, sexpr(sexpr(i, y), z), r);
        return s2;
    }
    }
    lean_unreachable();
    return sexpr();
}

sexpr format::best(unsigned w, unsigned k, sexpr const & s) {
    return be(w, k, sexpr{sexpr(0, s)}, sexpr());
}

std::ostream & operator<<(std::ostream & out, format const & f)
{
    return pretty(out, default_width, f);
}

format operator+(format const & f1, format const & f2) {
    return format{f1, f2};
}

format operator^(format const & f1, format const & f2) {
    return format{f1, format(" "), f2};
}

std::ostream & pretty(std::ostream & out, unsigned w, format const & f) {
    sexpr const & b = format::best(w, 0, f.m_value);
    return layout_list(out, b);
}
}
