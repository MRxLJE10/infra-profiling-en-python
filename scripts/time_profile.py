import time
from src.suma_primos import suma_primos

inicio = time.perf_counter()
suma_primos(10000)
duracion = (time.perf_counter() - inicio) * 1000  # convertir a ms

print(f"{duracion:.3f} ms")
