fn C (x) for var i = 1; true; i += 2: if i ** 2 > x: return i
var l c g x y z = int(I(day3.txt)), C(l) / 2, [], 0, 0, 0
pl abs(c - (l - (c * 2 - 1) ** 2) % (c * 2)) + c
for var i = 1; i <= l * l; i++ {
	var X Y = l / 2 + x, l / 2 - y
	if !x and !y: g[Y][X] = 1
	else g[Y][X] = (+) [
		g[Y + 1][X - 1], g[Y][X - 1], g[Y - 1][X - 1],
		g[Y + 1][X],     g[Y][X],     g[Y - 1][X],
		g[Y + 1][X + 1], g[Y][X + 1], g[Y - 1][X + 1]
	]
	if g[Y][X] > l { pl g[Y][X]; last }
	if i == (C(i) - 2) ** 2 + 1
	    or (+) [i == C(i) ** 2 - (C(i) - 1) * _ for 1 -> 3]:
		z = (z + 1) % 4
	match z { 0 => x++, 1 => y++, 2 => x--, 3 => y-- }
}
