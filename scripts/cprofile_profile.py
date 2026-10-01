"""Medir suma_primos(10000) con cProfile"""
import cProfile
import pstats
from src.suma_primos import suma_primos

perfil = cProfile.Profile()
perfil.runcall(suma_primos, 10000)
stats = pstats.Stats(perfil)

duracion = stats.total_tt * 1000  # convertir a ms
print(f"{duracion:.3f} ms")
