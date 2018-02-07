fn bar (x, y) = x * y

fn foo (x, y) {
	pl 'this is foo: ', bar(x, y)
	return max(x, y)
}
