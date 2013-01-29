def fibo (n)
	if n == 0 then
		return 1
	else if n == 1 then
		return 1
	else
		return fibo(n-1) + fibo(n-2)
	end
end

puts fibo(5)
puts fibo(10)
puts fibo(15)

