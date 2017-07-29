// for var i in 1 ~> 10: println i

var a = 5 ~> 3
println a
print "\n\n["

for var i = 0; i < length a; i++ {
	print a[i]
	if i != length a - 1: print ", "
}

say "]\n"

/*
fn fizzbuzz () {
	for var i = 1; i < 101; i++ {
		if i %% 3: print "Fizz"
		if i %% 5: print "Buzz"
		if i %% 3 nor i %% 5: print i
		print "\n"
	}
}

fn f (x) = x * 2
fn g (x y) = x * y
fn h (x) {
	println "h was called with a ", type x;
}

fizzbuzz()

var a = 10
while sayln a--;

h("test"), h(2), h(true)
for var i = 0; i < 10; i++:
	println i, " times ", f(i), " = ", g(i, f(i))
*/
