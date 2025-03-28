#include <memory>
#include <string>
#include <iostream>
#include <exception>


struct expression
{
    virtual ~expression() = default;
    virtual void dump(std::ostream &os) const = 0;
    virtual void dump2(std::ostream &os) const = 0;
    virtual bool has_blanks() const = 0;
    virtual std::string type_description() const = 0;
    // TODO: probably cna be removed after migration to shared pointers
    virtual std::shared_ptr<expression> duplicate() const = 0;
    // TODO: probably must be moved to a separate interface alongside w/ reduce
    virtual std::shared_ptr<expression> replace(const std::string &what, const expression &with_what) const = 0;
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
    void dump2(std::ostream &os) const override {
        os << name;
    }
    std::string type_description() const override {
        return "variable";
    }
    std::shared_ptr<expression> duplicate() const override {
        return std::make_shared<variable>(name);
    }
    std::shared_ptr<expression> replace(const std::string &what, const expression &with_what) const override {
        return what == name ? with_what.duplicate() : duplicate();
    }
};


struct func : expression
{
    func(std::string input, std::shared_ptr<expression> out = nullptr) : input(std::move(input)), out(out) {}
    std::string input;
    std::shared_ptr<expression> out = nullptr;
    bool has_blanks() const override {
        return !out;
    }
    void dump(std::ostream &os) const override {
        os << "(λ";
        os << input;
        if (auto out_as_func = std::dynamic_pointer_cast<func>(out); out_as_func) {
            out_as_func->dump_shorthand(os);
        } else {
            os << ".";
            if (out) out->dump(os); else os << "_";
        }
        os << ")";
    }
    void dump2(std::ostream &os) const override {
        os << "(";
        os << input;
        os << " -> ";
        if (out) out->dump2(os); else os << "_";
        os << ")";
    }
    std::string type_description() const override {
        return "function";
    }
    void dump_shorthand(std::ostream &os) const {
        os << input;
        if (auto out_as_func = std::dynamic_pointer_cast<func>(out); out_as_func) {
            out_as_func->dump_shorthand(os);
        } else {
            os << ".";
            if (out) out->dump(os); else os << "_";
        }
    }
    std::shared_ptr<expression> duplicate() const override {
        return std::make_shared<func>(input, out ? out->duplicate() : nullptr);
    }
    std::shared_ptr<expression> replace(const std::string &what, const expression &with_what) const override {
        auto replaced = std::dynamic_pointer_cast<func>(duplicate());
        replaced->out = replaced->out->replace(what, with_what);
        return replaced;
    }
    std::shared_ptr<expression> reduce(const expression &with_what) const {
        return out ? out->replace(input, with_what) : nullptr;
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
        if (foo) foo->dump(os); else os << "_";
        os << ' ';
        if (in) in->dump(os); else os << "_";
        os << ')';
    }
    void dump2(std::ostream &os) const override {
        if (foo) foo->dump2(os); else os << "_";
        os << '(';
        if (in) in->dump2(os); else os << "_";
        os << ')';
    }
    std::string type_description() const override {
        return "call";
    }
    std::shared_ptr<expression> duplicate() const override {
        return std::make_shared<call>(foo ? foo->duplicate() : nullptr, in ? in->duplicate() : nullptr);
    }
    std::shared_ptr<expression> replace(const std::string &what, const expression &with_what) const override {
        auto replaced = std::dynamic_pointer_cast<call>(duplicate());
        replaced->foo = replaced->foo->replace(what, with_what);
        replaced->in = replaced->in->replace(what, with_what);
        return replaced;
    }
    std::shared_ptr<expression> reduce() const {
        auto foo_as_func = std::dynamic_pointer_cast<func>(foo);
        return foo_as_func && !has_blanks() ? foo_as_func->reduce(*in) : duplicate();
    }
};


void print(std::ostream &os, const std::shared_ptr<expression> &e, const std::string title = "") {
    bool has_title = !title.empty();
    os << (has_title ? title : e->type_description());
    os << ":\n\t";
    e->dump(os);
    os << " -- " << e->type_description();
    os << '\n';

    os << "\t";
    e->dump2(os);
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


int main(int, char **)
{
    auto apply_x_to_b = new_func("x", new_call(new_variable("x"), new_variable("b")));
    auto call_apply_E_to_b = new_call(apply_x_to_b, new_variable("E"));
    auto reduced = call_apply_E_to_b->reduce();
    print(std::cout, call_apply_E_to_b, "1A. replace x with E");
    print(std::cout, reduced, "1B. reduced");

    auto to_reduce = new_call(
                new_func("x", new_call(new_variable("x"), new_call(new_variable("y"), new_variable("x")))),
                new_call(new_variable("f"), new_variable("f")));
    print(std::cout, to_reduce, "2A. to reduce");
    print(std::cout, to_reduce->reduce(), "2B. reduced");

    auto TRUE = new_func("a", new_func("b", new_variable("a")));
    auto FALSE = new_func("a", new_func("b", new_variable("b")));
    auto AND = new_func("x", new_func("y", new_call(new_call(new_variable("x"), new_variable("y")), FALSE)));
    print(std::cout, TRUE, "3A. True");
    print(std::cout, FALSE, "3B. False");
    print(std::cout, AND, "3C. x and y");
    return 0;
}
