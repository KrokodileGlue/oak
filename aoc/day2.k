var l s t = I(day2.txt), 0, 0

for split /\n/, l {
	next when !length
	var a b = -1, 0
	for split /\s+/ {
		a = int when a < 0
		b = int > b ? int : b
		a = int < a ? int : a
	}
	s += b - a
	a = 0, b = 0
	for var i; split /\s+/ {
		for var j; split /\s+/ {
			if int i %% int j and i != j:
				a = int(i), b = int(j)
			last when a
		}
		last when a
	}
 	t += a / b
}

pl s, '\n', t
