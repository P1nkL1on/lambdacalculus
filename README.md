Simple exercise to step a bit to lambda calculus

```bash
1A. replace x with E:
	((λx.(x b)) E) -- call
	(x -> x(b))(E) -- call
1B. reduced:
	(E b) -- call
	E(b) -- call

2A. to reduce:
	((λx.(x (y x))) (f f)) -- call
	(x -> x(y(x)))(f(f)) -- call
2B. reduced:
	((f f) (y (f f))) -- call
	f(f)(y(f(f))) -- call

3A. True:
	(λab.a) -- function
	(a -> (b -> a)) -- function
3B. False:
	(λab.b) -- function
	(a -> (b -> b)) -- function
3C. x and y:
	(λxy.((x y) (λab.b))) -- function
	(x -> (y -> x(y)((a -> (b -> b))))) -- function
```
