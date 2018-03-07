var a b c d e P = chomp(I(day7.txt)), [], split(/\n/, a), 0, -1, {}
for c: if /\w+ \(\d+\) -> (.*)/: for $1 =~ /\w+/: push b
for c: goto end when /^(\w+) / and !(+)[_ == $1 for b] ? d = $1 : false
fn f (x) for c: return int $2 when /^(\w+) \((\d+)\)/ and x == $1
fn g (x) for c: return $2 =~ /\w+/g when /^(\w+) \(\d+\) -> (.*)/ and x == $1
fn h (x) = P[x] = f(x) + (+)[h(_) for g(x)]
fn u (x) {
	var s freak = sort([P[_] for g(x)]), nil
	if s[0] != s[length s - 1] {
		if s[0] == s[1]: freak = s[length s - 1]
		else freak = s[0]
	}
	if freak: for g(x): if P[_] == freak { return _ }
}
fn i (x) {
	var U = u(x)
	if U and !u(U) { for g(x): if _ != U: e = f(U) + P[_] - P[U] }
	else if U: i(U)
}
end: h(d), i(d); pl '$d\n$e'
