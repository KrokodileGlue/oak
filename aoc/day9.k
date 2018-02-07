var a b c d e = I(day9.txt), 0, false, 0, 0
a =~ s/\s//g

for var i = 0; i < length a; i++ {
	if c and a[i] != '>' and a[i] != '!' {
		e++
		next
	}

	match a[i] {
		'{' => d += ++b,
		'}' => b--,
		'<' => c = true,
		'>' => c = false,
		'!' => i++
	}
}

pl '$d\n$e'
