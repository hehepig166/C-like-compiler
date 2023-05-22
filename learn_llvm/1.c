//#include <stdio.h>

// clang -emit-llvm test.cpp -S -o test.ll

int fib(int x) {
    if (x < 2) return 1;
    return fib(x-2) + fib(x-1);
}

int test_if(int x, int yy)
{
    int y;
    while (x > 1)
        y = y+1;
    return 0;
}

void test_call()
{
    int a, b;
    a = test_if(2, b);
}

int fun(int x)
{
    if (x > 0) return x*fun(x);
    return 1;
}


int if_def(int a, int b)
{
    int ret;
    int c = -100, e = -1000;

    if (a > 0 || b == 2) {
        int c = b*a;
        int d = c-5;
        ret = c+d;
    }
    else {
        int e = b/a;
        ret = e*2;
    }

    return ret;
}

int main()
{
    int ans;
    ans = if_def(-10, 2);

    //printf("%d\n", ans);

    return 0;
}