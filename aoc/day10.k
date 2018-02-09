var a = map { int } split /,/, I(day10.txt)

fn hash (x) {
	var h = 0 -> 255;
	var b = 0
	var s = 0

	for x {
		for var i = b; i < _ + b; i++ {
			var t = h[256 - i]
			h[i] = h[256 - i]
			h[256 - i] = t
		}

		b += _ + s
		s++
	}

	return h
}

pl join ', ', hash(a)
