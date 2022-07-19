# Product of Primes Between n and 2n

Finding counterexamples to a lemma found in some course notes online.

## Motivation
While reading some [course notes](https://ocw.mit.edu/courses/18-703-modern-algebra-spring-2013/resources/mit18_703s13_pra_l_22/) from MIT 18.703 (Modern Algebra), I stumbled upon this stated (but not proved) lemma:
**Lemma 22.3.** The product of all prime numbers r between N and 2N is greater than 2^N for all N ≥ 1.

However, it turns out that this lemma is false. One can quickly find a counterexample at `n = 8`: 11 times 13 = 143 < 256 = 2^8.

This lemma is important for the proof of **Lemma 22.7**, so I was curious to know if this lemma could be fixed by counting out some (small) finite number of counterexamples, and so I wrote the code in this project that day.

This is written in C++ and uses the Number Theory Library for its bignum support and OpenMP for some very basic parallelism. This code was run for n = 1 to 100000 on a computer with an AMD 3950X and 32 GB DDR4-3600 (14-16-16-32).

## Results for n between 1 and 100000.
```
Counterexample at n = 8
Product of primes between n and 2n = 143
2^n = 256

Counterexample at n = 14
Product of primes between n and 2n = 7429
2^n = 16384

Counterexample at n = 20
Product of primes between n and 2n = 765049
2^n = 1048576
```
