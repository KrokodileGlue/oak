fn f (x) = x * 2

var a = 'foo' + 'bar' + ((2 + 4) * 3 / 9 - 1 + 4)
pl a

var b = 2 * 2
pl (b + 6) + a
b = 2000

pl '2 * 2 = ', f(2)

fn g = 'stuff'
pl g()

var c = [1, 2, 3, 4]
pl c
c[0][5] = 0
c[0][4] = 1
c[50][0] = 2
pl c

var d = 0
fn h = d++

for var i = 0; i < 30; i++:
	pl h()

p 'checking if g() == "stuff"... '

if g() == 'stuff':
	pl '[TRUE]'
else
	pl '[FALSE]'

fn fib (x) {
	if x < 2: return x
	return fib(x - 1) + fib(x - 2)
}

for var i = 0; i < 20; i++:
	pl i, ': ', fib(i)

pl type a
pl type b
pl type c
pl type d

var dim = 50

fn square (dim) {
	var field = []

	for var y = 0; y < dim; y++:
		for var x = 0; x < dim; x++ {
			field[x][y] = ' '
			if y == x or y == dim - x:
				 field[x][y] = '#'
		}

	for var y = 0; y < dim; y++ {
		for var x = 0; x < dim; x++:
			p field[x][y]
		p '\n'
	}
}

fn ppm-square (dim) {
	pl 'P3'
	pl dim, ' ', dim
	pl dim

	for var y = 0; y < dim; y++:
		for var x = 0; x < dim; x++:
	 		p x + ' ' + y + ' 0 '

	p '\n'
}

square(dim)
# ppm-square(dim)

fn t (x) pl type x

t('this'), t('that'), t(2), t(true), t([1, 2, 'stuff']), t(1.2)

var n = 0
while n < 10: pl n++
n = 0
do pl n++; while n < 10

pl 'stuff: ', 3 * 3.1415926535897932

var array = [1, 2, 3, 4]

fn function (x) {
	x[0] = 'asdf'
	pl x
}

pl array
function(array)
pl array
pl length array
array[0] = ['abc', 'def', [0, 1, 2]]

for var i = 0; i < length array; i++:
	pl i + ': ' + array[i]

# fn join (x y) {
# 	return y when type y != 'array'
# 	var r
# 	for var i = 0; i < length y - 1; i++:
# 		r += y[i] + x
# 	return r + y[length y - 1]
# }

fn djoin (x y) {
	return y when type y != 'array'
	var r

	for var i = 0; i < length y - 1; i++ {
		if type y[i] == 'array': r += djoin(x, y[i]) + x
		else r += y[i] + x
	}

	return r + djoin(x, y[length y - 1])
}

pl join ', ', array
pl djoin(', ', array)

var it = [fn { pl type array }, 2, 3, 4]
it[0]()
pl it[1] + it[-500]

for var i = 0; i < 40; i++ {
	next when i %% 2
	pl i
	last when i == 19
}

fn scope {
	var hard-normal-daddy = 300

	fn internal (x) {
		p type x
		# error: undeclared identifier
		# pl hard-normal-daddy
	}

	pl 'this should say array: ', internal([])
}

scope()

# fn range(x y) {
# 	var ret
# 	for var i = 0; i <= y - x; i++:
# 		ret[i] = i + x
# 	return ret
# }

{
	for var i; range(5, 9): pl i

	pl '========'

	var i
	for i; range(5, 9): pl i

	pl '========'

	for range(5, 9): pl _

	pl '========'

	pl join(', ', map { 2 ** _ } range(0, 62))
}

pl join('-', split / /, 'this is something')

var string = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur dictum.'

for string =~ /\w+/g: pl _
pl join(':', string =~ /\w+/g)
pl join(':', split //, string)

if string =~ /^(.*?), (.*)$/:
	pl 'group 1: "' + $1 + '", group 2: "' + $2 + '"'

pl string =~ s/[aeiouy]+/\U$0/gi
pl eval "string =~ s/[aeiouy]+//gi"

pl join(', ', map { 2 ** _ } range(0, 12))
pl eval "join(', ', map { 2 ** _ } range(0, 12))"
eval "pl 'test'"

fn test-eval {
	eval "return join(':', split / /, 'this is test-eval returning.')"
}

pl test-eval(), ' ', test-eval()

for var i = 0; i < 10; i++ {
	eval "if i %% 2: next"
	pl i

	for var j = 0; j < 10; j++ {
		pl 'j: ', j
		eval "if j == 5: last"
	}

	eval "if i == 7 { last }"
}

# eval "next"

fn abs (x) = x < 0 ? -x : x

string = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur dictum.'
pl string =~ s/[aeiouy]+/"'o'"/eegi

string = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur dictum.'
pl string =~ s/(\w)(\w+)/uc $1 + join('-', split ||, $2)/egi

eval 'var __ = "this is an eval-scoped variable."; pl __'

# error: undeclared identifier
# eval 'pl __'

pl join(' ', map { reverse } split / /, 'this is test')
pl join(' ', map { ucfirst reverse } 'this is test' =~ /\w+/g)
pl join(' ', map { lc reverse } 'tHiS iS tEst' =~ /\w+/g)
pl reverse 'this is test'

pl join ', ', reverse range(0, 30)
pl join('\n', map { reverse join('-', split //) } ['foo', 'bar'])

pl join(':', map { sayln } range(0, 5))

var table = {
	a: 'some text',
	b: fn (self x) {
		self.a = x
	}
}

pl table['a']
table['b'](table, 5555)

pl table.a
table.b(1000)
pl table.a

fn Boat (name attack) = {
	name: name,
	health: 100,
	alive: true,
	attack: attack,

	x: 0,
	y: 0,
	z: 0,

	move: fn (self x y z) {
		self.x += x
		self.y += y
		self.z += z
	},

	show: fn (self) {
		pl 'The ', self.name, ' is at '
		    + self.x + ', ' + self.y + ', ' + self.z + '.'
	},

	damage: fn (self damage) {
		self.health = self.health - damage
		pl 'The ', self.name, ' takes ', damage, ' damage!'
		pl 'The ', self.name, ' now has ', self.health, ' health.'

		self.alive = false when self.health <= 0
	},

	is-alive: fn (self) {
		pl self.alive ? 'The ' + self.name + ' is still kicking.'
		 : 'The ' + self.name + ' is dead! RIP!'
	},

	fight: fn (self enemy) {
		pl 'The ', self.name, ' is now fighting the ', enemy.name, '!'

		if enemy.alive {
			pl 'The ', enemy.name, ' deals ', enemy.attack, ' damage!'
			self.damage(enemy.attack)
		} else pl 'The ', enemy.name, ' cannot fight because it is dead.'

		if self.alive and !enemy.alive {
			pl 'The ', self.name, ' is victorious.'
		} else self.is-alive()
	}
}

var boat1 = Boat('U.S.S Enterprise', 25)
var boat2 = Boat('S.S. Minnow', 30)

boat1.show()
boat1.move(0, 1, 0)
boat1.move(3, -30, 7)
boat1.show()

while boat1.alive and boat2.alive {
	boat1.fight(boat2)
	boat2.fight(boat1)
}

pl join ', ', range 0, 10, 5

# TODO: add int, str, etc. builtin function
# TODO: default arguments and varargs
# TODO: finish tables by robustizing
# TODO: actually implement the tracing garbage collector
# TODO: string subscripts and assignment
# TODO: fix ktre warnings
# TODO: && and >= operators, -= for tables, arrays
# TODO: 'and' and 'or' highlighting in oak-mode
# TODO: list comprehensions. see below.
# TODO: string interpolation

# pl 'string subscripts?'[0]

# pl 0 for range(0, 10)
# pl 2 * _ for range(0, 10)
# pl i for var i in range(0, 10)

# var j
# pl j for j in range(0, 10)
