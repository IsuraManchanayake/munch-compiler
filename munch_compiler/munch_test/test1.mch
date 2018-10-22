struct V {
    x: int; 
    y: int;
}

var v: V = {y=1, x=2}
var w: int[3] = {1, [2]=3}
var vv: V[2] = {{1, 2}, {3, 4}}
const w_ = (:V) {y = 1, x = 2}

func add(a: V, b: V): V { 
    c := V{0}; 
    c = {a.x + b.x, a.y + b.y}; 
    return c; 
}

func fib(n: int): int { 
    if(n <= 1) {
        return n;
    } 
    return fib(n - 1) + fib(n - 2);
}

