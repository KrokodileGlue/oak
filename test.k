fn f (x) = x * 2

var a = 'foo' + 'bar' + ((2 + 4) * 3 / 9 - 1 + 4)
println a

var b = 2 * 2
println (b + 6) + a

println '2 * 2 = ', f(2)

return 5
# class foo {
# 	var x y z

# 	# Maybe use a different syntax for methods than functions.
# 	# TODO: functions that take no arguments should not require ()
# 	fn foo () {
# 		x = 1
# 		y = 2
# 		z = 3
# 	}

# 	fn f () {
# 		return x + y + z
# 	}
# }

# var c = foo()
# println c.f()
# c.x = 2
# println c.f()
