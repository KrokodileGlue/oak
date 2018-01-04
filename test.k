fn f (x) = x * 2

var a = 'foo' + 'bar' + ((2 + 4) * 3 / 9 - 1 + 4)
println a

var b = 2 * 2
println (b + 6) + a
b = 2000

println '2 * 2 = ', f(2)

fn g = "stuff"
println g()

var c = [1, 2, 3, 4]
println c
c[0][5] = 0
c[0][4] = 1
c[50][0] = 2
println c

var d = 0
fn h = d++

for var i = 0; i < 30; i++:
	println h()

print 'checking if g() == "stuff"... '

if g() == "stuff":
	println "[TRUE]"
else
	println "[FALSE]"

fn fib (x) {
	if x < 2: return x
	return fib(x - 1) + fib(x - 2)
}

for var i = 0; i < 30; i++:
	println i, ': ', fib(i)

println type a
println type b
println type c
println type d

var dim = 50
var field = []

for var y = 0; y < dim; y++ {
	for var x = 0; x < dim; x++ {
		field[x][y] = " "
		if y == x or y == dim - x:
			 field[x][y] = "#"
	}
}

for var y = 0; y < dim; y++ {
	for var x = 0; x < dim; x++ {
		print field[x][y]
	}
	print "\n"
}

println 'test'

fn ppm_square (dimen) {
	println "P3"
	println dimen, " ", dimen
	println "1"

	for var y = 0; y < dimen; y++ {
		for var x = 0; x < dimen; x++ {
	 		if y == x or y == dimen - x:
				println "0 0 0"
			else println "1 1 1"
		}
	}
}

# ppm_square(dim)
