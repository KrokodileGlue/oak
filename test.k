//var a = ["this", "is", "a", "list", "with", "some", "stuff", "in", "it"]
//println a

/*
for var i = 1; i < 101; i++ {
	if i % 3 == 0 and i % 5 == 0:
		println "FizzBuzz"
	else if i % 3 == 0:
		println "Fizz"
	else if i % 5 == 0:
		println "Buzz"
	else
		println i
}
*/

fn f (x) = x * 2
fn g (x y) = x * y
fn h (x) {
	println "h was called with a ", type x
}
h("test"), h(2), h(true)

/*
for var i = 0; i < 10; i++:
	println i, " times ", f(i), " = ", g(i, f(i))
*/

var a = 10
while a: println a --
