"""Medir suma_primos(10000) con timeit, promediando 20 veces"""
import timeit
from src.suma_primos import suma_primos

N = 20
total = timeit.timeit(lambda: suma_primos(10000), number=N)
promedio = (total / N) * 1000  # convertir a ms

print(f"{promedio:.3f} ms")
