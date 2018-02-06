var l a b = I(day2.txt), 0, 0

for split /\n/, l {
	next when !length
	var c x y = map { int } _ =~ /\S+/g, 0, 0
	for var i; c: for var j; c: if i %% j and i != j: x = i, y = j
	a += max c - min c
 	b += x / y
}

pl a, '\n', b
