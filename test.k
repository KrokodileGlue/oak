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

println type a
println type b
println type c
println type d
