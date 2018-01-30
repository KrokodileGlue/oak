var l = I(day2.txt)
l =~ s/\s+\Z//s

var s = 0
for split /\n/, l {
	var mn = -1
	var mx = 0

	for split /\s+/ {
		mn = int when mn < 0
		mx = int > mx ? int : mx
		mn = int < mn ? int : mn
	}

	s += mx - mn
}

pl s

s = 0
for split /\n/, l {
	var a = 0
	var b = 0

	for var i; split /\s+/ {
		for var j; split /\s+/ {
			if int i %% int j and i != j:
				a = int(i), b = int(j)

			last when a
		}

		last when a
	}

	s += a / b
}

pl s
