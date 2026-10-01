// El producto punto de dos vectores de floats, dos veces: con un ciclo
// escalar y con instrucciones AVX que operan ocho floats por instrucción.
//
// Los valores son enteros pequeños y el vector tiene 2^20 posiciones, de
// modo que todas las sumas parciales son exactas en float y las dos versiones
// tienen que imprimir el mismo número.
#include <chrono>
#include <cstdio>
#include <immintrin.h>
#include <vector>

using namespace std;
using namespace std::chrono;

const size_t N = 1 << 20;
const int REPETICIONES = 200;

// Un producto y una suma por elemento, en secuencia.
float escalar(const float *a, const float *b, size_t n) {
  float suma = 0.0f;
  for (size_t i = 0; i < n; i++) suma += a[i] * b[i];
  return suma;
}

// TODO: de a ocho elementos por vuelta. Cargar ocho floats de a y ocho de b
// con _mm256_loadu_ps, multiplicarlos con _mm256_mul_ps y acumular en un
// registro __m256 con _mm256_add_ps. Al final, sumar las ocho posiciones del
// acumulador (reducción horizontal) y agregar los elementos que sobran cuando
// n no es múltiplo de ocho.
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
  __m128 alto = _mm256_extractf128_ps(acumulador, 1);   // [4, 5, 6, 7]
  __m128 bajo = _mm256_castps256_ps128(acumulador);     // [0, 1, 2, 3]
  __m128 s = _mm_add_ps(alto, bajo);                     // Suma [0+4, 1+5, 2+6, 3+7]
  s = _mm_hadd_ps(s, s);                                 // Suma parejas: [0+4+1+5, ...]
  s = _mm_hadd_ps(s, s);                                 // Una más: [resultado, ...]
  float suma = _mm_cvtss_f32(s);                         // Extrae el resultado
  
  // Cola: n % 8 elementos restantes
  for (; i < n; i++) {
    suma += a[i] * b[i];
  }
  
  return suma;
}

int main() {
  vector<float> a(N), b(N);
  for (size_t i = 0; i < N; i++) {
    a[i] = (float)(i % 4);
    b[i] = (float)(i % 3);
  }

  float r1 = 0, r2 = 0;
  auto t0 = high_resolution_clock::now();
  for (int k = 0; k < REPETICIONES; k++) r1 = escalar(a.data(), b.data(), N);
  auto t1 = high_resolution_clock::now();
  for (int k = 0; k < REPETICIONES; k++) r2 = con_avx(a.data(), b.data(), N);
  auto t2 = high_resolution_clock::now();

  printf("escalar %.1f ms resultado %.0f\n",
         duration_cast<microseconds>(t1 - t0).count() / 1000.0, r1);
  printf("avx %.1f ms resultado %.0f\n",
         duration_cast<microseconds>(t2 - t1).count() / 1000.0, r2);
  return 0;
}
