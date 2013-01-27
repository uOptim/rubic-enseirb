def f(a, b, c)
	a = 1
	b = 3.2
	c = true
	puts a
	puts b
	puts c
end

# should not compile
f(2, 3, 1)
# should print
# 1
# 3.2
# true
f(3, 4.2, false)
