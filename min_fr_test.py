import numpy as np
import math
import matplotlib.pyplot as plt

# Parameters (you can change these)
n = 50          # total number
maxp = 1e-6     # threshold

# Compute the Boolean result for each ma in [0, n]
ma_values = np.arange(0, n + 1)
holds = [
    math.factorial(m) * math.factorial(n - m) / math.factorial(n) <= maxp
    for m in ma_values
]

# Plot
plt.figure(figsize=(8, 4))
plt.scatter(ma_values, holds)
plt.xlabel('ma')
plt.ylabel('Inequality holds\n(1=True, 0=False)')
plt_title = f'Inequality'
plt.title(plt_title)
plt.ylim(-0.1, 1.1)
plt.grid(True)
plt.show()
