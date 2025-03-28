#include <memory>
#include <string>
#include <iostream>
#include <exception>


struct expression
{
    virtual ~expression() = default;
    virtual void dump(std::ostream &os) const = 0;
    virtual bool has_blanks() const = 0;
    virtual std::string type_description() const = 0;
};


struct variable : expression
{
    variable(std::string name) : name(std::move(name)) {}
    std::string name;
    bool has_blanks() const override {
        return false;
    }
    void dump(std::ostream &os) const override {
        os << name;
    }
    std::string type_description() const override {
        return "variable";
    }
};

enum {
    DUMP_LAMBDA,
    DUMP_ARROW,
}
static constexpr dump_format = DUMP_ARROW;

static constexpr bool shortdump = true;

struct func : expression
{
    func(std::string input, std::shared_ptr<expression> out = nullptr) : input(std::move(input)), out(out) {}
    std::string input;
    std::shared_ptr<expression> out = nullptr;
    std::string type_description() const override {
        return "function";
    }
    bool has_blanks() const override {
        return !out;
    }
    void dump(std::ostream &os) const override {
        os << '(';
        os << "λ";
        os << ' ' << input;
        if (auto out_as_func = std::dynamic_pointer_cast<func>(out); shortdump && out_as_func) {
            out_as_func->dump_shorthand(os);
        } else {
            os << (dump_format == DUMP_LAMBDA ? " . " : " -> ");
            if (out) out->dump(os); else os << "_";
        }
        os << ')';
    }
    void dump_shorthand(std::ostream &os) const {
        os << ' ' << input;
        if (auto out_as_func = std::dynamic_pointer_cast<func>(out); shortdump && out_as_func) {
            out_as_func->dump_shorthand(os);
        } else {
            os << (dump_format == DUMP_LAMBDA ? " . " : " -> ");
            if (out) out->dump(os); else os << "_";
        }
    }
};


struct call : expression
{
    call(std::shared_ptr<expression> foo = nullptr, std::shared_ptr<expression> in = nullptr) : foo(foo), in(in) {}
    std::shared_ptr<expression> foo = nullptr;
    std::shared_ptr<expression> in = nullptr;
    bool has_blanks() const override {
        return !foo || !in;
    }
    void dump(std::ostream &os) const override {
        os << '(';
        if (auto foo_as_call = std::dynamic_pointer_cast<call>(foo); shortdump && foo_as_call) {
            foo_as_call->dump_shorthand(os);
        } else {
            if (foo) foo->dump(os); else os << "_";
        }
        os << ' ';
        if (in) in->dump(os); else os << "_";
        os << ')';
    }
    void dump_shorthand(std::ostream &os) const {
        if (auto foo_as_call = std::dynamic_pointer_cast<call>(foo); shortdump && foo_as_call) {
            foo_as_call->dump_shorthand(os);
        } else {
            if (foo) foo->dump(os); else os << "_";
        }
        os << ' ';
        if (in) in->dump(os); else os << "_";
    }
    std::string type_description() const override {
        return "call";
    }
};


void print(std::ostream &os, const std::shared_ptr<expression> &e, const std::string title = "") {
    bool has_title = !title.empty();
    os << (has_title ? title : e->type_description());
    os << ":\n\t";
    if (e) e->dump(os); else os << "_";
    os << " -- " << e->type_description();
    os << '\n';
}

std::shared_ptr<variable> new_variable(std::string name) {
    return std::make_shared<variable>(std::move(name));
}

std::shared_ptr<func> new_func(std::string name, std::shared_ptr<expression> out = nullptr) {
    return std::make_shared<func>(std::move(name), out);
}

std::shared_ptr<call> new_call(std::shared_ptr<expression> foo = nullptr, std::shared_ptr<expression> in = nullptr) {
    return std::make_shared<call>(foo, in);
}

std::shared_ptr<expression> beta_reduce(std::shared_ptr<expression> expr);


std::shared_ptr<expression> replace_varname(
        std::shared_ptr<expression> where, const std::string &what, std::shared_ptr<expression> with, int d) {

    std::shared_ptr<expression> reduced;

    if (auto var = std::dynamic_pointer_cast<variable>(where); var) {
        reduced = var->name == what ? with : var;

    } else if (auto foo = std::dynamic_pointer_cast<func>(where); foo) {
        reduced = new_func(foo->input, replace_varname(foo->out, what, with, d + 1));

    } else if (auto fcall = std::dynamic_pointer_cast<call>(where); fcall) {
        reduced = new_call(
                    replace_varname(fcall->foo, what, with, d + 1),
                    replace_varname(fcall->in, what, with, d + 1));
    }
    if (!reduced) {
        throw std::runtime_error(where ? "where must be either variable, func or call" : "where can't be nullptr");
    }
    while (d--)
        std::cout << "__";
    std::cout << "beta reduce replace :: where=";
    where->dump(std::cout);
    std::cout << " what='" << what << "' with=";
    with->dump(std::cout);
    std::cout << " result=";
    reduced->dump(std::cout);
    std::cout << "\n";
    std::flush(std::cout);
    return reduced;
}

std::shared_ptr<expression> remove_repeat_params(std::shared_ptr<expression> expr) {
    auto foo = std::dynamic_pointer_cast<func>(expr);
    if (!foo) return expr;

    auto foo2 = std::dynamic_pointer_cast<func>(foo->out);
    if (!foo2) return expr;

    return (foo->input == foo2->input) ? foo2 : foo;
}

std::shared_ptr<expression> beta_reduce(std::shared_ptr<expression> expr) {
    auto fcall = std::dynamic_pointer_cast<call>(expr);
    if (!fcall) return expr;

    auto foo = std::dynamic_pointer_cast<func>(beta_reduce(fcall->foo));
    if (!foo) return expr;

    return beta_reduce(replace_varname(foo->out, foo->input, fcall->in, 1));
}


int main(int, char **)
{
    auto apply_x_to_b = new_func("x", new_call(new_variable("x"), new_variable("b")));
    auto call_apply_E_to_b = new_call(apply_x_to_b, new_variable("E"));
    auto reduced = beta_reduce(call_apply_E_to_b);
    print(std::cout, call_apply_E_to_b, "1A. replace x with E");
    print(std::cout, reduced, "1B. reduced");

    print(std::cout, new_func("x", new_func("y")), "2A. func of func");
    print(std::cout, new_call(new_call(nullptr, new_variable("x")), new_variable("y")), "2B. call of call");
    print(std::cout,
                new_call(new_call(new_call(new_func("x", new_func("y", new_func("z"))),
                new_variable("a")), new_variable("b")), new_variable("c")), "2C. double triple");

    auto to_reduce = new_call(
                new_func("x", new_call(new_variable("x"), new_call(new_variable("y"), new_variable("x")))),
                new_call(new_variable("f"), new_variable("f")));
    print(std::cout, to_reduce, "3A. to reduce");
    print(std::cout, beta_reduce(to_reduce), "3B. reduced");

    auto TRUE = new_func("a", new_func("b", new_variable("a")));
    auto FALSE = new_func("a", new_func("b", new_variable("b")));
    print(std::cout, TRUE, "4A. True");
    print(std::cout, FALSE, "4B. False");
    print(std::cout, beta_reduce(new_call(new_call(TRUE, new_variable("1")), new_variable("2"))), "4D. True(1, 2)");
    print(std::cout, beta_reduce(new_call(new_call(FALSE, new_variable("1")), new_variable("2"))), "4E. False(1, 2)");

    auto right_1 = new_func("a", new_func("b", new_func("c", new_call(new_call(new_variable("c"), new_variable("b")), new_variable("a")))));
    auto apply = new_call(new_call(new_call(right_1, new_variable("1")), new_variable("2")), new_variable("3"));
    print(std::cout, right_1, "5A. reoder");
    print(std::cout, apply, "5B. reorder(1, 2, 3)");
    print(std::cout, beta_reduce(apply), "5C. reorder(1, 2, 3) = 3, 2, 1");

    auto apply_map = new_func("a", new_call(new_variable("a"), new_variable("x")));
    auto capitalize_b = new_call(apply_map, new_func("b", new_call(new_variable("b"), new_variable("b"))));
    print(std::cout, apply_map, "6A. map");
    print(std::cout, capitalize_b, "6B. map the map");
    print(std::cout, beta_reduce(capitalize_b), "6C. reduce map the map");

    auto NOT = new_func("x", new_call(new_call(new_variable("x"), FALSE), TRUE));
    print(std::cout, NOT, "7A. not");
    print(std::cout, beta_reduce(new_call(NOT, TRUE)), "7B. not(True)");
    print(std::cout, beta_reduce(new_call(NOT, FALSE)), "7C. not(False)");

    /// ((λ a b -> a) (λ a b -> b) (λ a b -> a))
    /// ((λ a b -> b)  (λ a b -> a))
    /// ((λ a b -> b)  (λ a b -> a))
    ///
    ///
    /// ((λ a b -> b) (λ a b -> b) (λ a b -> a))

    std::cout << '\n';
    return 0;
}
