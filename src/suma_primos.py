def es_primo(num):
    """Devuelve True si num es primo. Implementación deliberadamente ingenua."""
    if num < 2:
        return False
    for i in range(2, int(num**0.5) + 1):
        if num % i == 0:
            return False
    return True


def suma_primos(n):
    """Suma todos los primos menores que n."""
    return sum(i for i in range(2, n) if es_primo(i))


def suma_primos_rapida(n):
    """Usa la Criba de Eratóstenes: marca múltiplos en una sola pasada."""
    if n <= 2:
        return 0

    # es primo[i] será True si i es primo, False si no lo es
    es_primo = [True] * n
    es_primo[0] = es_primo[1] = False  # 0 y 1 no son primos

    # marcar múltiplos de cada primo encontrado
    for i in range(2, int(n**0.5) + 1):
        if es_primo[i]:
            for multiplo in range(i * i, n, i):
                es_primo[multiplo] = False

    # sumar todos los primos encontrados
    return sum(i for i in range(2,n) if es_primo[i])
