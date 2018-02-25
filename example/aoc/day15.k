var a b c d e f = 679, 771, 679, 771, 0, 0

for var i = 0; i < 40000000; i++ {
	a = (a * 16807) % 2147483647, b = (b * 48271) % 2147483647
	e++ when (a & 0xFFFF) == (b & 0xFFFF)
	if i < 5000000 {
		do c = (c * 16807) % 2147483647; while c % 4
		do d = (d * 48271) % 2147483647; while d % 8
		f++ when (c & 0xFFFF) == (d & 0xFFFF)
	}
}

pl '$e\n$f'
