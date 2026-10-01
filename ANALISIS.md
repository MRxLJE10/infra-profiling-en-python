# Análisis de Profiling en Python e Instrucciones AVX

Nombre y código: Juan Sebastian Perez 2459371

---

## Parte 1: cuatro herramientas, cuatro números

| Herramienta | Tiempo de `suma_primos(10000)` |
|---|---:|
| `time` | 23.588 ms |
| `timeit` (promedio de 20) | 12.344 ms |
| `cProfile` | 19.664 ms |
| `pyinstrument` | 8.636 ms |

### ¿Por qué no dan lo mismo?

Las cuatro herramientas miden de formas distintas:

- **`time.perf_counter()`**: Una sola muestra del reloj de alta resolución. El sistema operativo puede interferir, procesos en segundo plano, caché frío/caliente. No es confiable para una sola corrida.

- **`timeit`**: Promedia 20 ejecuciones del mismo código. Más confiable porque los saltos entre corridas se promedian. Da un número representativo del tiempo "típico".

- **`cProfile`**: Cuenta cada llamada a función. Agrega **overhead** porque registra cada entrada y salida. Por eso es un poco más lento que `time` y `timeit`.

- **`pyinstrument`**: Muestrea la pila cada milisegundo (intervalo). Tiene menos overhead que `cProfile` porque no monitorea cada llamada, solo toma muestras. Pero es el número más bajo.

### ¿Cuál se acerca más al tiempo real?

**`timeit` es el más confiable** para este caso. Es un promedio de varias ejecuciones, así que filtra el ruido del sistema. `time` es una sola muestra y puede ser engañosa. `cProfile` es preciso pero lento. `pyinstrument` es rápido pero basado en muestreo.

---

## Parte 2: lo que mostró el perfil

### Función que concentra el tiempo

Salida de `cProfile` sobre `suma_primos(10000)`:

```
ncalls  tottime  percall  cumtime  percall filename:lineno(function)
 9998    0.008    0.000    0.008    0.000 suma_primos.py:1(es_primo)
 1230    0.002    0.000    0.010    0.000 suma_primos.py:13(<genexpr>)
```

**`es_primo` es el cuello de botella**: Se llamó **9998 veces** y consumió **0.008 segundos** (80% del tiempo total).

### Qué se cambió en `suma_primos_rapida`

**Implementación original (ingenua):**
```python
def es_primo(num):
    if num < 2:
        return False
    for i in range(2, int(num**0.5) + 1):
        if num % i == 0:
            return False
    return True

def suma_primos(n):
    return sum(i for i in range(2, n) if es_primo(i))
```

Para cada número `k` que queremos probar, recorremos desde 2 hasta √k. Con `n = 10000`, eso es 9998 llamadas a `es_primo`.

**Cambio: Criba de Eratóstenes**
```python
def suma_primos_rapida(n):
    if n <= 2:
        return 0
    
    # es_primo[i] es True si i es primo
    es_primo = [True] * n
    es_primo[0] = es_primo[1] = False
    
    # Marcar múltiplos de cada primo
    for i in range(2, int(n**0.5) + 1):
        if es_primo[i]:
            for multiplo in range(i * i, n, i):
                es_primo[multiplo] = False
    
    # Sumar los que quedaron marcados como True
    return sum(i for i in range(2, n) if es_primo[i])
```

**¿Por qué ataca lo que el perfil mostró?**

En vez de 9998 **llamadas a funciones**, hace una sola **pasada sobre un arreglo**:
- Complejidad original: O(n√n)
- Complejidad con Criba: O(n log log n)

El perfil mostró que `es_primo` se llamaba demasiadas veces. La Criba elimina esas llamadas reemplazándolas con operaciones directas sobre un arreglo.

### Resultados de la optimización

| Versión | Tiempo con `n = 200000` |
|---|---:|
| `suma_primos` | 356.3 ms |
| `suma_primos_rapida` | 17.2 ms |

**Factor: 20.8x**

---

## Parte 3: el producto punto tres veces

Dos vectores de **un millón de enteros** (`np.int64`).

### Tabla de resultados

| Forma | Tiempo |
|---|---:|
| Indexando el arreglo de NumPy | 252.20 ms |
| Ciclo sobre listas | 61.81 ms |
| NumPy vectorizado | 1.15 ms |

**Factor global: 53.9x** (indexado vs NumPy)

### ¿Por qué indexar NumPy desde Python es la más lenta?

Cuando haces `a[i]` en un arreglo de NumPy:
1. Se extrae un `np.int64` (escalar de NumPy) de la memoria
2. Se convierte a `int` de Python
3. Se operan dos `int` de Python (costo en cada iteración)
4. Se suma el resultado

Con un millón de elementos, esto es costoso.

### ¿Qué hace NumPy por debajo que el ciclo de Python no puede?

**NumPy:**
- El arreglo es un bloque de memoria homogéneo (`int64` de 64 bits)
- `np.dot` hace un **ciclo en C puro** sobre ese bloque
- **Sin crear objetos intermedios** por elemento
- Una sola pasada sobre memoria contiguaLa multiplicación y suma ocurren en el procesador sin volver a Python

**Ciclo de Python:**
- Cada iteración accede a `xs[i]` (puntero a un objeto `int`)
- Extrae el valor
- Hace la operación
- Suma
- Costo por elemento: varias instrucciones máquina

**Ciclo con indexación de NumPy:**
- Lo peor de ambos: crea un escalar de NumPy por elemento + conversión a Python + operación

NumPy vectorizado gana porque todo está en C, sin ir y venir a Python.

---

## Parte 4: AVX a mano

Vector de **2²⁰ floats** (1,048,576 elementos). 200 repeticiones.

### Tabla de resultados

| Versión | Tiempo | Resultado |
|---|---:|---:|
| Escalar | 284.1 ms | 1572863 |
| AVX | 120.4 ms | 1572863 |

**Factor: 2.36x**

### Implementación AVX

```cpp
float con_avx(const float *a, const float *b, size_t n) {
  __m256 acumulador = _mm256_setzero_ps();  // [0, 0, 0, 0, 0, 0, 0, 0]
  
  size_t i = 0;
  // Ciclo vectorial: de a 8 elementos por vuelta
  for (; i + 8 <= n; i += 8) {
    __m256 va = _mm256_loadu_ps(a + i);      // Cargar 8 floats de a
    __m256 vb = _mm256_loadu_ps(b + i);      // Cargar 8 floats de b
    __m256 prod = _mm256_mul_ps(va, vb);     // Multiplicar posición a posición
    acumulador = _mm256_add_ps(acumulador, prod);  // Acumular
  }
  
  // Reducción horizontal: sumar las 8 posiciones del acumulador
  __m128 alto = _mm256_extractf128_ps(acumulador, 1);
  __m128 bajo = _mm256_castps256_ps128(acumulador);
  __m128 s = _mm_add_ps(alto, bajo);
  s = _mm_hadd_ps(s, s);
  s = _mm_hadd_ps(s, s);
  float suma = _mm_cvtss_f32(s);
  
  // Cola: n % 8 elementos restantes
  for (; i < n; i++) {
    suma += a[i] * b[i];
  }
  
  return suma;
}
```

### ¿Por qué no llega a 8x aunque cada instrucción opera 8 floats?

El teórico máximo sería 8x (8 floats por instrucción). Pero hay limitaciones reales:

1. **Cadena de dependencia de datos**: Cada iteración suma el resultado a `acumulador`, y la siguiente iteración necesita ese resultado. La CPU no puede lanzar la siguiente suma hasta que la anterior termine (latencia ≈ 3 ciclos).

2. **Cargas de memoria**: Hay que cargar 8 + 8 = 16 floats por vuelta. La memoria no es infinitamente rápida.

3. **Reducción horizontal**: Al final, sumar 8 números es secuencial y tiene costo.

4. **Cola**: Los n % 8 elementos que sobran se suman en modo escalar.

**Para mejorar:**
- Usar **múltiples acumuladores independientes** (la CPU puede trabajar en paralelo en ellos)
- Mejorar la localidad de caché
- Desenrollar más el ciclo

Con una cadena, el techo es n / 3 ≈ 3.3x. Con múltiples acumuladores desacoplados se puede llegar a 5-6x.

---

## Resumen general

| Parte | Optimización | Factor |
|---|---|---|
| 1 | Medición | 4 herramientas |
| 2 | Criba de Eratóstenes | **20.8x** |
| 3 | NumPy vectorizado | **53.9x** |
| 4 | Instrucciones AVX | **2.36x** |

Cada nivel baja más, pero con diminishing returns. NumPy vectorizado es el cambio más espectacular porque reemplaza un ciclo de Python con C puro.