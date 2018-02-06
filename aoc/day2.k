var l a b = I(day2.txt), 0, 0

for split /\n+/, l {
	var c x y = map { int } _ =~ /\S+/g, 0, 0
	for var i; c: for var j; c: if i %% j and i != j: b += i / j
	a += max c - min c
}

pl a, '\n', b
