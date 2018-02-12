fn D (x) = str [hex(>> 4) + hex(& 0xF) for x]
fn f (x, z) {
	var h a b = 0 -> 255, 0, 0
	for 1 -> z: for x {
		var r = reverse h[a:a +]
		if r: for 0 ->: h[(a +) % 256] = r[]
		a += _ + 1 + b++
	}
	return h
}
fn g (x) {
	var h r t = f(x, 64), [], 0
	for var i = 0; i < length h; i += 16 {
		t = h[i]
		for var j = i + 1; j < i + 16; j++: t = t ^ h[j]
		push r, t
	}
	return r
}
var c = f(map { int() - 1 } split(/,/, I(day10.txt)), 1)
pl c[0] * c[1]
pl D(g(map { _ - 1 } ord chomp I(day10.txt) + [17, 31, 73, 47, 23]))
