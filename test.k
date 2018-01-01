fn f (x) = x * 2

var a = 'foo' + 'bar' + ((2 + 4) * 3 / 9 - 1 + 4)
println a

var b = 2 * 2
println (b + 6) + a

println '2 * 2 = ', f(2)

fn g = "stuff"
println g()

var c = [1, [2, "asdf"], 3, b, 5, [6, g()], 7]
println c
