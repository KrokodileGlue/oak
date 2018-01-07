fn f (x) = x * 2

var a = 'foo' + 'bar' + ((2 + 4) * 3 / 9 - 1 + 4)
pl a

var b = 2 * 2
pl (b + 6) + a
b = 2000

pl '2 * 2 = ', f(2)

fn g = 'stuff'
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

if g() == 'stuff':
	pl '[TRUE]'
else
	pl '[FALSE]'

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
			field[x][y] = ' '
			if y == x or y == dim - x:
				 field[x][y] = '#'
		}

	for var y = 0; y < dim; y++ {
		for var x = 0; x < dim; x++:
			p field[x][y]
		p '\n'
	}
}

fn ppm-square (dim) {
	pl 'P3'
	pl dim, ' ', dim
	pl dim

	for var y = 0; y < dim; y++:
		for var x = 0; x < dim; x++:
	 		p x + ' ' + y + ' 0 '

	p '\n'
}

square(dim)
# ppm-square(dim)

fn t (x) pl type x

t('this'), t('that'), t(2), t(true), t([1, 2, 'stuff']), t(1.2)

var n = 0
while n < 10: pl n++
n = 0
do pl n++; while n < 10

pl 'stuff: ', 3 * 3.1415926535897932

var array = [1, 2, 3, 4]

fn function (x) {
	x[0] = 'asdf'
	pl x
}

pl array
function(array)
pl array
pl length array
array[0] = ['abc', 'def', [0, 1, 2]]

for var i = 0; i < length array; i++:
	pl i + ': ' + array[i]

fn join (x y) {
	var r
	for var i = 0; i < length y - 1; i++:
		r += y[i] + x
	return r + y[length y - 1]
}

fn djoin (x y) {
	return y when type y != 'array'
	var r

	for var i = 0; i < length y - 1; i++ {
		if type y[i] == 'array': r += djoin(x, y[i]) + x
		else r += y[i] + x
	}

	return r + djoin(x, y[length y - 1])
}

pl join(', ', array)
pl djoin(', ', array)

# TODO: fix invalid memory access when variable initalizers are ruined
# by "when"
var it = [fn { pl type array }, 2, 3, 4]
it[0]()
pl it[1] + it[-500]

for var i = 0; i < 40; i++ {
	next when i % 2 == 0
	pl i
	last when i == 19
}

fn scope {
	var hard-normal-daddy = 300

	fn internal (x) {
		p type x
		# error: undeclared identifier
		# pl hard-normal-daddy
	}

	pl 'this should say array: ', internal([])
}

scope()

fn range(x y) {
	var ret = []
	for var i = 0; i < y - x; i++:
		ret[i] = i + x
	return ret
}

{
	for var i; range(5, 10): pl i

	pl '========'

	var i
	for i; range(5, 10): pl i

	pl '========'

	for range(5, 10): pl _

	pl '========'

	pl join(', ', map { _ * _ } range(0, 10))
}

# var foo = { a => 2, b => 'stuff' }
# pl foo['a']
