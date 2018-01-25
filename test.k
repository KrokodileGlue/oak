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

fn djoin (x, y) {
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
	a = 'some text',
	b = fn (self, x) {
		self.a = x
	},
	uninitialized-field
}

pl table['a']
table['b'](table, 5555)

pl table.a
table.b(1000)
pl table.a

if !table.uninitialized-field:
	pl 'table.uninitialized-field is nil.'

fn Boat (name = 'U.S.S. Nameless', attack = 20) = {
	name = name,
	health = 100,
	alive = true,
	attack = attack,

	x = 0,
	y = 0,
	z = 0,

	move = fn (self, x, y, z) {
		self.x += x
		self.y += y
		self.z += z
	},

	show = fn (self) {
		pl 'The ', self.name, ' is at '
		    + self.x + ', ' + self.y + ', ' + self.z + '.'
	},

	damage = fn (self, damage) {
		self.health = self.health - damage
		pl 'The ', self.name, ' takes ',        damage, ' damage!'
		pl 'The ', self.name, ' now has ', self.health, ' health.'

		self.alive = (self.health >= 0)
	},

	is-alive = fn (self) {
		self.alive = (self.health >= 0)
		pl self.alive ? 'The ' + self.name + ' is still kicking.'
			      : 'The ' + self.name + ' is dead! RIP!'
	},

	fight = fn (self, enemy) {
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

var boat1 = Boat('U.S.S. Enterprise', 25)
var boat2 = Boat('S.S. Minnow', 30)
var boat3 = Boat()
boat2.fight(boat3)

boat1.show()
boat1.move(0, 1, 0)
boat1.move(3, -30, 7)
boat1.show()

while boat1.alive and boat2.alive {
	boat1.fight(boat2)
	boat2.fight(boat1)
}

pl join ', ', range 0, 24.5, 5
pl join ', ', range 0.1, 24.5, 5
pl join ', ', range 0, 24.5, 5

var arr = [1, 2, 3, 4]
arr[3] += 10
arr[3] -= 4
pl arr[3]

boat2.is-alive()
boat2.health -= 100
boat2.is-alive()

var _str = 'string subscripts?'

if !_str[500] and !_str[-1]:
	pl 'invalid subscripts produce nil.'

pl map { _str[] } range 7, 16

var tstr = _str
pl tstr
pl _str[6] = '-'
pl tstr
pl type _str

var asdf = [1, 2, 3, [4, 5, 6]]
var temp = asdf[3]
temp[0] = 500
pl join ', ', asdf;

pl join ', ', push asdf, 7
map { push asdf } range 8, 10
pl join ', ', asdf

pl join ', ', map { (2 *) + } range 1, 5

# error: string requires integer subscript (got nil)
# pl _str[]

pl 5 * 'test'
pl join ', ', 5 * ['foo', 'bar']

pl int '-0x100' * 2
pl float '3.1415926535897932' * 3
pl (str 400) * 2

var x
pl join ', ', [0 for range 0, 9]
pl join ', ', [2 * x for x in range 0, 9]
pl join ', ', [3 * y for var y in range 0, 9]
pl join ', ', [2 * for [2 * for range 0, 9]]

fn arg1 (a, b, c, d = 3) {
	pl d
}

arg1(1, 2, 3)
arg1(1, 2, 3, 'd')

fn arg2 (a, b, c, d = ...) {
	pl '====== argtest ======'

	pl a, ' (', type (a), ')'
	pl b, ' (', type (b), ')'
	pl c, ' (', type (c), ')'
	pl join (', ', d), ' (', type (d), ')'
}

arg2(1, 2, 3)
arg2(1, 2, 3, 4, 5, 6, 7)
arg2(1)

pl string

string =~ s/-//g
string = ucfirst lc string
string =~ s/(?<=\.\s+)(\w)/\u$1/

pl string
pl match string {
	/asdf/ => 'asdf!',

	/(.*?)amet/ => {
		pl 'Statement!'
		'thing: ' + $1
	},

	true => +{
		foo = 'this is a table!',
		bar = 'tables can be returned from matches.'
	},

	true => 'Goodbye, world!',
	true => 'Hello, world!'
}

pl '===== Advent of Code ====='
pl '       == Day 1 =='
var input = '29917128875332952564321392569634257121244516819997569284938677239676779378822158323549832814412597817651244117851771257438674567254146559419528411463781241159837576747416543451994579655175322397355255587935456185669334559882554936642122347526466965746273596321419312386992922582836979771421518356285534285825212798113159911272923448284681544657616654285632235958355867722479252256292311384799669645293812691169936746744856227797779513997329663235176153745581296191298956836998758194274865327383988992499115472925731787228592624911829221985925935268785757854569131538763133427434848767475989173579655375125972435359317237712667658828722623837448758528395981635746922144957695238318954845799697142491972626942976788997427135797297649149849739186827185775786254552866371729489943881272817466129271912247236569141713377483469323737384967871876982476485658337183881519295728697121462266226452265259877781881868585356333494916519693683238733823362353424927852348119426673294798416314637799636344448941782774113142925315947664869341363354235389597893211532745789957591898692253157726576488811769461354938575527273474399545366389515353657644736458182565245181653996192644851687269744491856672563885457872883368415631469696994757636288575816146927747179133188841148212825453859269643736199836818121559198563122442483528316837885842696283932779475955796132242682934853291737434482287486978566652161245555856779844813283979453489221189332412315117573259531352875384444264457373153263878999332444178577127433891164266387721116357278222665798584824336957648454426665495982221179382794158366894875864761266695773155813823291684611617853255857774422185987921219618596814446229556938354417164971795294741898631698578989231245376826359179266783767935932788845143542293569863998773276365886375624694329228686284863341465994571635379257258559894197638117333711626435669415976255967412994139131385751822134927578932521461677534945328228131973291962134523589491173343648964449149716696761218423314765168285342711137126239639867897341514131244859826663281981251614843274762372382114258543828157464392'
p 'part 1: '
print sum [input[] == (input * 2)[1 +] ? int input[] : 0 for 0 -> length input]
# print(sum(map(int, [a for a, b in zip(input, input[-1:] + input[:-1]) if a == b])))

# Part 2
p 'part 2: '
pl eval join '+', [input[] == (input + input)[length input / 2 +] ? input[] : 0 for range 0, length input]

pl '       == Day 2 =='
input = R($)
1364	461	1438	1456	818	999	105	1065	314	99	1353	148	837	590	404	123
204	99	235	2281	2848	3307	1447	3848	3681	963	3525	525	288	278	3059	821
280	311	100	287	265	383	204	380	90	377	398	99	194	297	399	87
7698	2334	7693	218	7344	3887	3423	7287	7700	2447	7412	6147	231	1066	248	208
3740	837	4144	123	155	2494	1706	4150	183	4198	1221	4061	95	148	3460	550
1376	1462	73	968	95	1721	544	982	829	1868	1683	618	82	1660	83	1778
197	2295	5475	2886	2646	186	5925	237	3034	5897	1477	196	1778	3496	5041	3314
179	2949	3197	2745	1341	3128	1580	184	1026	147	2692	212	2487	2947	3547	1120
460	73	52	373	41	133	671	61	634	62	715	644	182	524	648	320
169	207	5529	4820	248	6210	255	6342	4366	5775	5472	3954	3791	1311	7074	5729
5965	7445	2317	196	1886	3638	266	6068	6179	6333	229	230	1791	6900	3108	5827
212	249	226	129	196	245	187	332	111	126	184	99	276	93	222	56
51	592	426	66	594	406	577	25	265	578	522	57	547	65	564	622
215	2092	1603	1001	940	2054	245	2685	206	1043	2808	208	194	2339	2028	2580
378	171	155	1100	184	937	792	1436	1734	179	1611	1349	647	1778	1723	1709
4463	4757	201	186	3812	2413	2085	4685	5294	5755	2898	200	5536	5226	1028	180$

input =~ s/^\n+//

# KTRE bug?
# input =~ s/\n+$//

var s = 0
for split /\n/, input {
	var min = -1
	var max = 0

	for split /\s+/ {
		min = int when min < 0
		max = int > max ? int : max
		min = int < min ? int : min
	}

	# pl 'min: ', min, ', max: ', max
	s += max - min
}

pl 'part 1: ', s

s = 0
for split /\n/, input {
	# Redeclaration of identifier?
	a = 0
	b = 0

	for var i; split /\s+/ {
		for var j; split /\s+/ {
			# Shitty operator precedence issue with = and ,.
			if int i %% int j and i != j:
				(a = int i), (b = int j)

			last when a
		}

		last when a
	}

	# pl 'a: ', a, ', b: ', b
	s += a / b
}

pl 'part 2: ', s

# TODO: finish tables by robustizing
# TODO: actually implement the tracing garbage collector
# TODO: fix ktre warnings
# TODO: string interpolation
# TODO: regex matches shouldn't survive function calls
