source = '''

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

struct V(?) {
    x: int;
    y: int;
}

union IntOrPtr(?) {
    i: int;
    p: int*;
}

const xx(?) = 42
var zero(?): V(?) = {x = 0, y = 0}
var one(?) = (:V(?)) {x = 1, y = 1}
var zero_(?): V(?) = {x = 0}
var basis(?): V(?)[2] = {{0, cast(int, 1.0)}, {1, 0}}

typedef T1(?) = V(?)*[sizeof(:V(?))]
typedef T2(?) = S(?)*
const yy(?) = sizeof(vec_add(?)(zero(?), one(?)))

struct S(?) {
    i: int;
    f: float;
    c: char;
}

struct Student(?) {
    index(?): int;
}

func fib(?)(n: int): int {
    if(n <= 1) {
        return n;
    }
    return fib(?)(n - 1) + fib(?)(n - 2);
}

func do_abs_nothing(?)() {
    ;;;;;;;;
    return;
}

func vec_add(?)(a: V(?), b: V(?)): V(?) {
    return {a.x + b.x, a.y + b.y};
}

func norm(?)(a: V(?)): int {
    return a.x * a.x + a.y * a.y;
}

func mul(?)(a: V(?), k: int): V(?) {
    return {k * a.x, k * a.y};
}

typedef vec_decay_t(?) = func(V(?)):int

func adj_sum(?)(a: V(?), f: vec_decay_t(?)):int {
    // s := 0
    var s = 0;
    for(i := 0; i < 2; i++) {
        s += f(vec_add(?)(a, basis(?)[i]));
        s += f(vec_add(?)(mul(?)(a, -1), basis(?)[i]));
    }
    return s;
}

enum Dirs(?) {
    up(?),
    down(?) = 10,
    left(?),
    right(?),
    blah(?) = 'A'
}

func grade(?)(marks: int): int {
    if(marks >= 75) {
        return 'A';
    } else if(marks >= 50) {
        return 'B';
    } else if(marks >= 25) {
        return 'C';
    }
    return 'F';
}

func do_nothing(?)(grade: int) {
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
            do_abs_nothing(?)();
        }
    }
    do_abs_nothing(?)();
}

func facto_while(?)(n: int): int {
    i := 1;
    p := 1;
    while(i <= n) {
        p *= i;
        i++;
    }
    return p;
}

func facto_do_while(?)(n: int): int {
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

func facto_rec(?)(n: int): int {
    return n <= 1 ? 1 : n * facto_rec(?)(n - 1);
}

func is_prime(?)(n: int): int {
    for(i := 2; i * i <= n; i++) {
        if(n % i == 0) {
            return false;
        }
    }
    return true;
}

'''

N = 1 << 10
with open('test{}.mch'.format(N), 'w') as out_f:
    for i in range(N):
        out_f.write(source.replace('(?)', str(i)))