fn d (x) = str [hex(>> 4) + hex(& 0xF) for x]

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

var a = ''
pl sum [
	length (
		(a .= str (map { [_ & 2 ** i ? '#' : '.' for var i in range(7, 0, -1)] }
			g(map { _ - 1 } ord ('stpzcrnm-' + _)
			+ [17, 31, 73, 47, 23])) + '\n') =~ /#/g)
	for 0 -> 127
]

a =~ s/\s+\Z//
var b c = 0, map { split //, _ } split(/\n/, a)

fn h (x, y) {
	c[x][y] = b
	if c[x - 1] and c[x - 1][y] == '#': h(x - 1, y)
	if c[x][y - 1] == '#': h(x, y - 1)
	if c[x][y + 1] == '#': h(x, y + 1)
	if c[x + 1] and c[x + 1][y] == '#': h(x + 1, y)
}

for var i; 0 -> 127: for var j; 0 -> 127 {
	if c[i][j] == '#' {
		h(i, j)
		b++
	}
}

pl b
