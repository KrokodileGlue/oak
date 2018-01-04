fn f (x) = x * 2

var a = 'foo' + 'bar' + ((2 + 4) * 3 / 9 - 1 + 4)
pl a

var b = 2 * 2
pl (b + 6) + a
b = 2000

pl '2 * 2 = ', f(2)

fn g = "stuff"
pl g()

var c = [1, 2, 3, 4]
pl c
c[0][5] = 0
c[0][4] = 1
c[50][0] = 2
pl c

var d = 0
fn h = d++

for var i = 0; i < 30; i++:
	pl h()

p 'checking if g() == "stuff"... '

if g() == "stuff":
	pl "[TRUE]"
else
	pl "[FALSE]"

fn fib (x) {
	if x < 2: return x
	return fib(x - 1) + fib(x - 2)
}

for var i = 0; i < 20; i++:
	pl i, ': ', fib(i)

pl type a
pl type b
pl type c
pl type d

var dim = 50

fn square (dim) {
	var field = []

	for var y = 0; y < dim; y++:
		for var x = 0; x < dim; x++ {
			field[x][y] = " "
			if y == x or y == dim - x:
				 field[x][y] = "#"
		}

	for var y = 0; y < dim; y++ {
		for var x = 0; x < dim; x++:
			p field[x][y]
		p "\n"
	}
}

fn ppm_square (dim) {
	pl "P3"
	pl dim, " ", dim
	pl dim

	for var y = 0; y < dim; y++:
		for var x = 0; x < dim; x++:
	 		p x + " " + y + " 0 "

	p "\n"
}

square(dim)
# ppm_square(dim)

fn t (x) pl type x

t("this"), t("that"), t(2), t(true), t([1, 2, "stuff"]), t(1.2)

var n = 0
while n < 10: pl n++
n = 0
do pl n++; while n < 10

pl "stuff: ", 3 * 3.1415926535897932

# var foo = { a => 2, b => 'stuff' }
# pl foo["a"]

var array = [1, 2, 3, 4]

fn function (x) {
	x[0] = "asdf"
	pl x
}

pl array
function(array)
pl array
