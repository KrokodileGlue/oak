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

var a = [1, 2, 3, 4, 5]
println a
a[0][0] = "test"
println a
a = "foobar"
println a
a[0] = [1, 2, 3], a[1] = ["foo", "bar", "baz"]
println a

// fizzbuzz()

// var a = 10
// while sayln a--;

// h("test"), h(2), h(true)
// for var i = 0; i < 10; i++:
// 	println i, " times ", f(i), " = ", g(i, f(i))
