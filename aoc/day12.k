var a b c = I(day12.txt), [], []
a =~ s/\s+\Z//s

for split /\n/, a {
	/(\d+) <-> (.*)/
	var i = int $1
	b[i] = []
	for $2 =~ /\d+/g:
		push b[i], int
}

fn f (x) {
	for c: return 0 when x == _
	push c, x
	return sum [f(b[x][]) for range(0, length(b[x]) - 1)] + 1
}

pl f(0)
pl sum [1 iff f(_) != 0 for range(0, length b - 1)] + 1
