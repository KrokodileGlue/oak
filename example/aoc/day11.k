var x y mx my = 0, 0, 0, 0

for I(day11.txt) =~ /\w+/ {
	mx = x when x > mx
	my = y when y > my

	match _ {
		'n' => y++,
		's' => y--,
		'ne' => (x++, y++),
		'se' => x++,
		'nw' => x--,
		'sw' => (x--, y--)
	}
}

pl max(abs(x), abs(y), abs(x - y))
pl max(abs(mx), abs(my), abs(mx - my))
