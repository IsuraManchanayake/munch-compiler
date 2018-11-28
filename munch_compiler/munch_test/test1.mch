

// comment 1
// comment 2 // //
/*
 comment 3
*/

/**
 * comment 4
 * 
 */

/**
 ***  // //
 */

 
var qq0: int
var rr0: int = qq0 + 1
var ss0: int[2]
var tt0 = (:int[2][2]){{1, 2}, ss0}
var uu0 = (:int[2][2][1]){tt0}

func do_n() {
    p := 10;
    a := p + (rr0 + 1);
    qq0 = 10;
}
/*
struct V0 {
    x: int;
    y: int;
}

union IntOrPtr0 {
    i: int;
    p: int*;
}

const xx0 = 1 + 0 * 0
var zero0: V0 = {x = 0, y = 0}
var one0 = (:V0) {x = 1, y = 1}
var zero_0: V0 = {x = 0}
var basis0: V0[2] = {{0, cast(int, 1.0)}, {1, 0}}
var vv0: V0 = {0}

typedef T1_0 = V0*[sizeof(:V0)]
typedef T2_0 = S0*
const yy0 = sizeof(vec_add0(zero0, one0))
const zz0 = (1 + 2 << 3 + 4 % 5 - 6 & 7 + 8 * 9 * 0) / (-10)

struct S0 {
    i: int;
    f: float;
    c: char;
}

struct Student0 {
    index0: int;
}

func fib0(n: int): int {
    if(n <= 1) {
        return n;
    }
    return fib0(n - 1) + fib0(n - 2);
}

func do_abs_nothing0() {
    ;;;;;;;;
    return;
}

func vec_add0(a: V0, b: V0): V0 {
    return {a.x + b.x, a.y + b.y};
}

func norm0(a: V0): int {
    return a.x * a.x + a.y * a.y;
}

func mul0(a: V0, k: int): V0 {
    return {k * a.x, k * a.y};
}

typedef vec_decay_t0 = func(V0):int

func adj_sum0(a: V0, f: vec_decay_t0):int {
    // s := 0
    var s = 0;
    for(i := 0; i < 2; i++) {
        s += f(vec_add0(a, basis0[i]));
        s += f(vec_add0(mul0(a, -1), basis0[i]));
    }
    return s;
}

enum Dirs0 {
    up0,
    down0 = 10,
    left0,
    right0,
    blah0 = 'A'
}

func grade0(marks: int): int {
    if(marks >= 75) {
        return 'A';
    } else if(marks >= 50) {
        return 'B';
    } else if(marks >= 25) {
        return 'C';
    }
    return 'F';
}

func do_nothing0(grade: int) {
    switch(grade) {
        case 'A': {
            break;
        }
        case 'B': {
            break;
        }
        case 'C': {
        }
        case 'F': {
        }
        default: {
            do_abs_nothing0();
        }
    }
    do_abs_nothing0();
}

func facto_while0(n: int): int {
    i := 1;
    p := 1;
    while(i <= n) {
        p *= i;
        i++;
    }
    return p;
}

func facto_do_while0(n: int): int {
    i := 1;
    p := 1;
    do {
        p *= i;
        i++;
        if(i == n) {
            break;
        }
    } while(i <= n);
    return p;
}

func facto_rec0(n: int): int {
    return n <= 1 ? 1 : n * facto_rec0(n - 1);
}

func is_prime0(n: int): int {
    for(i := 2; i * i <= n; i++) {
        if(n % i == 0) {
            return false;
        }
    }
    return true;
}
*/