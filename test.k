/*

for i in 1 -> 100 {
	if i % 15 == 0:		print "fizzbuzz"
	else if i % 5 == 0:	print "fizz"
	else if i % 3 == 0:	print "buzz"
	else			print i
}
*/

/*
while true {
	print "stuff"
}
*/

for var i in 1 -> 100 {
	var s = "";
	if !(i % 3): s += "Fizz"
	if !(i % 5): s += "Buzz"
	print s == "" ? i : s
}

/*
fn foo-buzz {
	fn f x {
		if i % 15 == 0:	      yield "fizzbuzz"
		else if i % 5 == 0:   yield "fizz"
		else if i % 3 == 0:   yield "buzz"
		else		      yield i
	}

	for var i = 1; i < 101; i++: print f (i)
}

-f(a = -85 * 92 - 23 ? b++ = (25 * (22 + 38)) : --c = 63) * 43;

foo-buzz(1, 2, 3)
foo-buzz(4, 5, 6)
fn f x { yield x * 2; }

(foo-buzz * 2)(1, 2, 3), 2 * 5
*/
#while