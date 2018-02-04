var a b c = [], 0, 0
for split /\n/, I(day13.txt): if /(\d+): (\d+)/: a[int $1] = int $2
for var i = 0; i < length a; i++:
	b += i * a[i] when a[i] != nil and i % ((a[i] - 1) * 2) == 0
n: c++
for var i = 0; i < length a; i++:
	goto n when a[i] != nil and (c + i) % ((a[i] - 1) * 2) == 0
pl b, '\n', c
