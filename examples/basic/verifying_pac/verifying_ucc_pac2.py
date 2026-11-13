# Пример -- в колабе. А здесь чисто логика, чтобы не ждать, пока замёржим алгоритм.
# Если Вы видите этот файл в репозитории, значит миру пришёл конец, b ,jkmit ybxnj yt bcnbyyj

from matplotlib import pyplot as plt

import desbordante

TABLE = "examples/datasets/verifying_pac/laptop_score.csv"

algo = desbordante.pac_verification.algorithms.UCCPACVerifier()
algo.load_data(table=(TABLE, ",", True), column_indices=[1, 2], delta_steps=int(1e6))

DELTA_STEPS = int(1e5)
delta_step = 1 / DELTA_STEPS

deltas = [i * delta_step for i in range(DELTA_STEPS)]
epsilons: list[float] = []
for delta in deltas:
    algo.execute(max_epsilon=0, min_delta=delta)
    pac = algo.get_pac()
    epsilons.append(pac.epsilon)

plt.plot(epsilons, deltas)
plt.show()
