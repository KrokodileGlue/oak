var s = 0

for split /\n/, I(day4.txt) {
	next when length == 0
	var w = split /\s+/
	var win = true
	for var i; range 0, length w: for var j; range 0, length w:
		win = false when w[i] == w[j] and i != j
	s++ when win
}

pl s
s = 0

for split /\n/, I(day4.txt) {
	next when length == 0
	var w = split /\s+/
	var win = true
	for var i; range 0, length w: for var j; range 0, length w:
		win = false when i != j and sort w[i] == sort w[j]
	s++ when win
}

pl s
