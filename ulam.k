#!/usr/bin/env oak

fn main (argv = ...) {
	var g s x y z = [], int(argv[0]), 0, 0, 0
	fn c (x) for var i = 1; true; i += 2: if i ** 2 > x: return i
	for var i = 1; i <= s * s; i++ {
		g[s / 2 - y][s / 2 + x] = i
		if i == (c(i) - 2) ** 2 + 1
		    or (+)[i == c(i) ** 2 - _ * (c(i) - 1) for 1 -> 3]:
			z = (z + 1) % 4
		match z { 0 => x++, 1 => y++, 2 => x--, 3 => y-- }
	}
	pl join '\n', [[rjust(_, length str(s * s) + 1) for _] for g]
}
