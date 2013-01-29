a = 1
def a
	return 2
end

def b
	return 3
end

# should print 1
puts a
# should print 2
puts a()
# should print 3
puts b
# should print 3
puts b()
