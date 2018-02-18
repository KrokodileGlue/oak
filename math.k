fn sqrt (x) = 2
fn cos (x) = 2
fn log2 (x) = 2
fn smoothstep (x, y, z) = 2
fn mix (x, y, z) = 2

/* vectors */
enum VEC2, VEC3, VEC4

fn vec = {
	len = fn (self) = match self.kind {
		VEC2 => m::sqrt(self.x ** 2 + self.y ** 2),
		VEC3 => m::sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2),
		VEC4 => m::sqrt(self.x ** 2 + self.y ** 2 + self.z ** 2 + self.w ** 2)
	}
}

fn vec2 (x = 0.0, y = 0.0) = vec() + {
	kind = VEC2,
	x = x,
	y = y
}

fn vec3 (x = 0.0, y = 0.0, z = 0.0) = vec() + {
	kind = VEC3,
	x = x,
	y = y,
	z = z
}

fn vec4 (x = 0.0, y = 0.0, z = 0.0, w = 0.0) = vec() + {
	kind = VEC4,
	x = x,
	y = y,
	z = z,
	w = w
}

fn dot (l, r) = match l.kind {
	VEC2 => l.x * r.x + l.y * r.y,
	VEC3 => l.x * r.x + l.y * r.y + l.z * r.z,
	VEC4 => l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w
}
