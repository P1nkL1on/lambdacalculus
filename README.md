Simple exercise to step a bit to lambda calculus https://www.youtube.com/watch?v=RcVA8Nj6HEo

```bash
> substitute ((λx.(xb))E) -> (Eb)
  ((λx.(xb))E)
  (Eb)
> function w/ output independent from input
  ((λx.y)a)
  y
> substitute func w/ 3 args
  (λxyz.foo)
  ((λxyz.foo)abc)
  foo
> true and false selectors
  (λab.a)
  (λcd.d)
  ((λab.a)12)
  1
  ((λcd.d)12)
  2
> scope of reducing
  (λa.(aaa))
  ((λa.(aaa))(λa.(aaa)))
  ((λa.(aaa))(λa.(aaa))(λa.(aaa)))
> not boolean function
  (λx.(x(λcd.d)(λab.a)))
> not true == false
  ((λx.(x(λcd.d)(λab.a)))(λab.a))
  ((λab.a)(λcd.d)(λab.a))
  (λcd.d)
> not false == true
  ((λx.(x(λcd.d)(λab.a)))(λcd.d))
  ((λcd.d)(λcd.d)(λab.a))
  (λab.a)
> and boolean function
> true & true == true
  (λab.a)
> true & false == false
  (λcd.d)
> false & true == false
  (λcd.d)
> false & false == false
  (λcd.d)
```
