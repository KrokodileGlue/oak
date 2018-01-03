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
fn h = (d = d + 1)

println h()
println h()
println h()
println h()

print 'checking if g() == "stuff"... '

if g() == "stuff":
	println "[TRUE]"
else
	println "[FALSE]"

fn fib (x) {
	if x == 1:
		return 1
	else if x == 0:
		return 0

	return fib(x - 1) + fib(x - 2)
}

println fib(0)
println fib(1)
println fib(2)
println fib(3)
println fib(4)
println fib(5)
println fib(6)
println fib(7)
println fib(8)
println fib(9)
println fib(10)
println fib(11)
println fib(12)
println fib(13)
println fib(14)
println fib(15)
println fib(16)
println fib(17)

println type a
println type b
println type c
println type d
