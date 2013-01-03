def h(e)
	return 1
end

class A
	def f(a)
		b = a
		return a + 1.2
	end
	def z(a)
		return a + 1.2
	end
end

class B < A
	def g(b, c)
		return b
	end
end

