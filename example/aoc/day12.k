var a b = [], []

for split /\n/, I(day12.txt) {
	next when !/(\d+) <-> (.*)/
	var i = int $1
	for $2 =~ /\d+/: push a[i], int
}

fn f (x) {
	for b: return 0 when x == _
	push b, x
	return sum [f(a[x][]) for 0 -> a[x]] + 1
}

pl f(0), '\n', sum [1 iff f(_) != 0 for 0 -> a] + 1
