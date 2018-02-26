fn sqrt (x) = 2
fn cos (x) = 2
fn log2 (x) = 2
fn smoothstep (x, y, z) = 2
fn mix (x, y, z) = 2

/* vectors */
enum VEC2, VEC3, VEC4

fn vec (x, y, z, w) = {
	x = x,
	y = y,
	z = z,
	w = w,

	_init_ = fn (self) {
		if x == nil:
			die "math::vec does not support vectors of less than two or greater than four dimensions"
		if y != nil: self.kind = VEC2
		if z != nil: self.kind = VEC3
		if w != nil: self.kind = VEC4
	},

	_len_ = fn (self) = match self.kind {
		VEC2 => m::sqrt(self.x ** 2 + self.y ** 2),
		VEC3 => m::sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2),
		VEC4 => m::sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2 + self.w ** 2)
	},

	_mul_ = fn (l, r) {
		if l.kind != r.kind:
			die "vector multiplication requires arguments of matching dimensions"
		match l.kind {
			VEC2 => { return vec(l.x * r.x, l.y * r.y) },
			VEC3 => { return vec(l.x * r.x, l.y * r.y, l.z * r.z) },
			VEC4 => { return vec(l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w) }
		}
	}
}

fn dot (l, r) = match l.kind {
	VEC2 => l.x * r.x + l.y * r.y,
	VEC3 => l.x * r.x + l.y * r.y + l.z * r.z,
	VEC4 => l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w
}
