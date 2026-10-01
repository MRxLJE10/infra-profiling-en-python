"""Medir suma_primos(10000) con pyinstrument y guardar HTML"""
from pyinstrument import Profiler
from src.suma_primos import suma_primos

perfilador = Profiler()
perfilador.start()
suma_primos(10000)
sesion = perfilador.stop()

duracion = sesion.duration * 1000  # convertir a ms
print(f"{duracion:.3f} ms")

# Guardar el informe HTML
with open("pyinstrument_results.html", "w") as f:
    f.write(perfilador.output_html())