class A
  def f(a)
    return a + 1.2
  end
end
object = A.new()
b = object.f(3)
class B < A
  def g(a)
    return a + 1.5
  end
end
