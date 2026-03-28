#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <optional>
#include <cassert>
#include <sstream>
#include <list>

using namespace std;

struct expression
{
    virtual ~expression() = default;
    virtual bool is_finished() const = 0;
    virtual void print(ostream &os, bool allow_shorthand = false) const = 0;
    virtual shared_ptr<expression> with_substituted_var(int id, shared_ptr<expression> arg) const = 0;
    virtual shared_ptr<expression> beta_reduced() const { return nullptr; }
};

struct lambda {};

struct expr
{
    expr(string var, int id);
    expr(lambda, expr in, expr out);
    expr(expr foo, expr arg);
    bool if_finished() const { return e_ && e_->is_finished(); }
    void print(ostream &os, bool allow_shorthand = true) const { if (!e_) return; e_->print(os, allow_shorthand); }
    void beta_reduce() { if (!e_) return; auto reduced = e_->beta_reduced(); e_ = reduced ? reduced : e_; }
private:
    expr(expression *e) : e_(e) {}
    shared_ptr<expression> e_;
};

struct variable : expression
{
    variable(string c, int id) : c(c), id(id) {}
    variable(int id) : id(id) {}
    optional<string> c;
    int id = 0;
    bool is_finished() const override { return true; }
    void print(ostream &os, bool) const override {
        if (c) { os << *c; return; }
        os << char(id + 'a');
    }
    shared_ptr<expression> with_substituted_var(int id, shared_ptr<expression> arg) const override {
        return (id == this->id) ? arg : nullptr;
    }
};

struct function : expression
{
    function(shared_ptr<variable> in, shared_ptr<expression> out) : in(in), out(out) {}
    shared_ptr<variable> in;
    shared_ptr<expression> out;
    bool is_finished() const override { return in && out; }
    void print(ostream &os, bool allow_shorthand) const override {
        os << "(λ";
        in->print(os, allow_shorthand);
        auto out_f = out;
        if (allow_shorthand) {
            auto next_f = dynamic_pointer_cast<function>(out);
            while (next_f) {
                next_f->in->print(os, allow_shorthand);
                out_f = next_f->out;
                next_f = dynamic_pointer_cast<function>(next_f->out);
            }
        }
        os << ".";
        out_f->print(os, allow_shorthand);
        os << ")";
    }
    shared_ptr<expression> call_on_arg(shared_ptr<expression> arg) const {
        auto out_s = out->with_substituted_var(in->id, arg);
        return out_s ? out_s : out;
    }
    shared_ptr<expression> with_substituted_var(int id, shared_ptr<expression> arg) const override {
        assert(id != in->id && "each function must have independent variable");
        auto out_s = out->with_substituted_var(id, arg);
        return make_shared<function>(in, out_s ? out_s : out);
    }
};

struct call : expression
{
    call(shared_ptr<expression> foo, shared_ptr<expression> arg) : foo(foo), arg(arg) {}
    shared_ptr<expression> foo;
    shared_ptr<expression> arg;
    bool is_finished() const override { return foo && arg; }
    void print(ostream &os, bool allow_shorthand) const override {
        os << "(";
        list<string> args_stack;
        {
            stringstream s;
            arg->print(s, allow_shorthand);
            args_stack = {s.str()};
        }
        auto last_f = foo;
        if (allow_shorthand) {
            auto next_c = dynamic_pointer_cast<call>(foo);
            while (next_c) {
                stringstream s;
                next_c->arg->print(s, allow_shorthand);
                args_stack.push_front(s.str());
                last_f = next_c->foo;
                next_c = dynamic_pointer_cast<call>(next_c->foo);
            }
        }
        last_f->print(os, allow_shorthand);
        for (const string &arg : args_stack) os << arg;
        os << ")";
    }
    shared_ptr<expression> with_substituted_var(int id, shared_ptr<expression> arg) const override {
        auto foo_s = this->foo->with_substituted_var(id, arg);
        auto arg_s = this->arg->with_substituted_var(id, arg);
        return make_shared<call>(foo_s ? foo_s : foo, arg_s ? arg_s : this->arg);
    }
    shared_ptr<expression> beta_reduced() const override {
        assert(is_finished());
        auto foo_br = foo->beta_reduced();
        auto foo_r = dynamic_pointer_cast<function>(foo_br ? foo_br : foo);
        if (!foo_r) return nullptr;

        auto arg_br = arg->beta_reduced();
        auto reduced = foo_r->call_on_arg(arg_br ? arg_br : arg);
        if (!reduced) return make_shared<call>(foo_r, arg_br ? arg_br : arg);

        return reduced;
    }
};

expr::expr(string var, int id) : e_(make_shared<variable>(var, id)) {}

expr::expr(lambda, expr in, expr out) : e_(make_shared<function>(dynamic_pointer_cast<variable>(in.e_), out.e_)) {}

expr::expr(expr foo, expr arg) : e_(make_shared<call>(foo.e_, arg.e_)) {}


int main(int, char **)
{
    {
        cout << "> substitute ((λx.(xb))E) -> (Eb)\n";
        expr x("x", 0);
        expr b("b", 1);
        expr e("E", 2);
        expr xxbe(expr(lambda{}, x, expr(x, b)), e);

        cout << "  "; xxbe.print(cout); cout << '\n';

        xxbe.beta_reduce();
        cout << "  "; xxbe.print(cout); cout << '\n';
    }
    {
        cout << "> function w/ output independent from input\n";
        expr c(expr(lambda{}, expr("x", 0), expr("y", 1)), expr("a", 2));
        cout << "  "; c.print(cout); cout << '\n';

        c.beta_reduce();
        cout << "  "; c.print(cout); cout << '\n';
    }
    {
        cout << "> substitute func w/ 3 args\n";
        expr x("x", 0);
        expr y("y", 1);
        expr z("z", 2);
        expr foo("foo", 3);
        expr xyzfoo(lambda{}, x, expr(lambda{}, y, expr(lambda{}, z, foo)));

        cout << "  "; xyzfoo.print(cout); cout << '\n';

        expr abc(expr(expr(xyzfoo, expr("a", 4)), expr("b", 5)), expr("c", 6));
        cout << "  "; abc.print(cout); cout << '\n';

        abc.beta_reduce();
        cout << "  "; abc.print(cout); cout << '\n';
    }
    {
        cout << "> true and false selectors\n";

        expr bool_true(lambda{}, expr("a", 0), expr(lambda{}, expr("b", 1), expr("a", 0)));
        cout << "  "; bool_true.print(cout); cout << '\n';

        expr bool_false(lambda{}, expr("c", 2), expr(lambda{}, expr("d", 3), expr("d", 3)));
        cout << "  "; bool_false.print(cout); cout << '\n';
    }

    return 0;
}
