fn f (x) = x * 2

var a = 'foo' + 'bar' + ((2 + 4) * 3 / 9 - 1 + 4)
println a

var b = 2 * 2
println (b + 6) + a

println '2 * 2 = ', f(2)

fn g = "stuff"
println g()

var c = [1, 2, 3, 4]
println c

# class foo {
# 	var x y z

# 	# Maybe methods should have a different syntax than functions.
# 	fn foo {
# 		x = 1
# 		y = 2
# 		z = 3
# 	}

# 	fn f {
# 		return x + y + z
# 	}
# }

# var c = foo()
# println c.f()
# c.x = 2
# println c.f()
