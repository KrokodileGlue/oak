var b s = 0, 0

fn f (h, x, y) {
	for x {
		var r = reverse h[b:b + _]
		if r: for 0 -> _: h[(b +) % y] = r[]
		b += _ + 1 + s++
	}

	return h
}

fn dig (x) = x < 10 ? str x : match x {
	10 => 'a', 11 => 'b', 12 => 'c',
	13 => 'd', 14 => 'e', 15 => 'f'
}

fn g (x, y) {
	var h = 0 -> 255
	for 0 -> 63: h = f(h, x, y)
	return h
}

fn d (x) = str [dig(_ >> 4) + dig(_ & 0xF) for x]

fn t (x) {
	var ret = []

	for var i = 0; i < length x; i += 16 {
		var thing = x[i]
		for var j = i + 1; j < i + 16; j++:
			thing = thing ^ x[j]
		push ret, thing
	}

	return ret
}

var p1 = f(0 -> 255, map { int() - 1 } split(/,/, I(day10.txt)), 256, 0, 0)
b = s = 0

pl p1[0] * p1[1]
pl d(t(g(map { _ - 1 } ord(str(I(day10.txt) =~ /\S/g)) + [17, 31, 73, 47, 23], 256)))
