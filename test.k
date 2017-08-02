fn fizzbuzz () {
	for 1 ~> 100 {
		if i %% 3: print "Fizz"
		if i %% 5: print "Buzz"
		if i %% 3 nor i %% 5: print i
		print "\n"
	}
}

fn f (x) = x * 2
fn g (x y) = x * y
fn h (x) {
	println 'x = {x}, type(x) = {type x}'
}

# fizzbuzz()

h("test"), h(2), h(true)
for 0 ~> 9:
	println '{i} x {f(i)} = {g(i, f(i))}'

var a = 'Hello, '
println 'a = \'{a}\'\ntype a = {type a}\n\'{a + "world!"}\''
