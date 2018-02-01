var a x y mx my = I(day11.txt), 0, 0, 0, 0
a =~ s/\s//g

for a =~ /\w+/g {
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
