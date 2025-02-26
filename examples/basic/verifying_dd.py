import desbordante
a = desbordante.dd_verification.createDF("Col0", 0, 2)
b = desbordante.dd_verification.createDF("Col1", 0, 1)
lhs = [a]
rhs = [b]
c = desbordante.dd_verification.createDD(lhs, rhs)
print(c.__str__())