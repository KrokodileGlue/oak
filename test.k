fn f (x) = x * 2

var a = 'foo' + 'bar' + ((2 + 4) * 3 / 9 - 1 + 4)
println a

var b = 2 * 2
println (b + 6) + a
b = 2000

println '2 * 2 = ', f(2)

fn g = "stuff"
println g()

var c = [1, [2, "asdf"], 3, b, 5, [6, g()], 7]
println c
c[0][0] = "---"
c[0][1] = "???"
c[1][1] = "___________________"
println c

var d = 0
fn h = (d = d + 1)

println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
println h()
