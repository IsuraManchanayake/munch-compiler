uint64_t _r64_a, _r64_d, _r64_b, _r64_c, _r64_f, _r64_lower_mask, _r64_upper_mask;
enum {
    _r64_n = 312,
    _r64_w = 64,
    _r64_m = 156,
    _r64_r = 31,
    _r64_u = 29,
    _r64_s = 17,
    _r64_t = 37,
    _r64_l = 43
};
uint64_t _r64_MT[_r64_n];
uint64_t _r64_index = _r64_n + 1;

void seed_mt(uint64_t seed) {
    _r64_index = _r64_n;
    _r64_MT[0] = seed;
    for (size_t i = 1; i < _r64_n; i++) {
        _r64_MT[i] = _r64_f * (_r64_MT[i - 1] ^ (_r64_MT[i - 1] >> (_r64_w - 2)) + i);
    }
}

bool rand64_inited = false;

void init_rand64(void) {
    if (rand64_inited) return;
    _r64_a = 0xB5026F5AA96619E9;
    _r64_d = 0x5555555555555555ull;
    _r64_b = 0x71D67FFFEDA60000ull;
    _r64_c = 0xFFF7EEE000000000ull;
    _r64_f = 6364136223846793005ull;
    _r64_lower_mask = (1ull << _r64_r) - 1;
    _r64_upper_mask = ~_r64_lower_mask;
    seed_mt(0);
    rand64_inited = true;
}

void twist(void) {
    for (size_t i = 0; i < _r64_n; i++) {
        uint64_t x = (_r64_MT[i] & _r64_upper_mask) + (_r64_MT[(i + 1) % _r64_n] & _r64_lower_mask);
        uint64_t xA = x >> 1;
        if (x % 2) {
            xA ^= _r64_a;
        }
        _r64_MT[i] = _r64_MT[(i + _r64_m) % _r64_n] ^ xA;
    }
    _r64_index = 0;
}

uint64_t rand64(void) {
    if (_r64_index >= _r64_n) {
        if (_r64_index > _r64_n) {
            perror("Generator was never seeded");
            exit(5489);
        }
        twist();
    }
    uint64_t y = _r64_MT[_r64_index];
    y ^= (y >> _r64_u) & _r64_d;
    y ^= (y << _r64_s) & _r64_b;
    y ^= (y << _r64_t) & _r64_c;
    y ^= y >> _r64_l;
    _r64_index++;
    return y;
}

uint64_t rand64_range(uint64_t a, uint64_t b) {
    assert(b - a > 0);
    return a + rand64() % (b - a);
}

void rand_test(void) {
    printf("----- rand.c -----\n");
    init_rand64();
    enum { N = 1 << 8 };
    size_t A[N] = { 0 };
    size_t n_samples = N << 2;
    for (size_t i = 0; i < n_samples; i++) {
        int64_t rr = rand64_range(0, N);
        A[rr]++;
    }
    for (size_t i = 0; i < N; i++) {
        printf("%zu\t: ", i);
        size_t expected = n_samples / N;
        size_t chars = (A[i] << 5 * (expected >= 32)) / expected;
        for (size_t j = 0; j < chars; j++) {
            putchar('*');
        }
        putchar('\n');
    }
}
