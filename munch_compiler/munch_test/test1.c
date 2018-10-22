// Forward declarations
typedef struct V V;
V add(V a, V b);
int fib(int n);
int main(void);

// Defintions
struct V {
  int x;
  int y;
};
V v = {.y = 1, .x = 2};
int w[3] = {1, [2] = 3};
V vv[2] = {{1, 2}, {3, 4}};
const V w_ = (V) {.y = 1, .x = 2};
V add(V a, V b) {
  V c = (V) {0};
  c = (V) {((a).x) + ((b).x), ((a).y) + ((b).y)};
  return c;
}
int fib(int n) {
  if ((n) <= (1)) {
    return n;
  }
  return ((fib)((n) - (1))) + ((fib)((n) - (2)));
}
const int a = (1) % (3);
int main(void) {
  
}
