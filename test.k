/*
for i in 1 -> 100 {
	if i % 5 == 0 and i % 3 == 0:	print "fizzbuzz"
	else if i % 5 == 0:		print "fizz"
	else if i % 3 == 0:		print "buzz"
	else				print i;
}
*/

fn fizzbuzz = {
	for var i = 1; i < 101; i++ {
		if i % 5 == 0 and i % 3 == 0:	print "fizzbuzz"
		else if i % 5 == 0:		print "fizz"
		else if i % 3 == 0:		print "buzz"
		else				print i
	}
}

(fizzbuzz * 2)(1, 2, 3), 2 * 5
