var s t = 0, 0

for split /\n/, I(day4.txt) {
	next when !length
	var w = split /\s+/
	var win1 win2 = true, true

	for var i; 0 -> length w - 1: for var j; 0 -> length w - 1 {
		win1 = false when w[i] == w[j] and i != j
		win2 = false when i != j and sort w[i] == sort w[j]
		last when !win1 and !win2
	}

	s++ when win1
	t++ when win2
}

pl s, '\n', t
