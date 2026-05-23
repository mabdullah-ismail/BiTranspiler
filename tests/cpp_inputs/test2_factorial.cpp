int factorial(int n) {
    int result = 1;
    int i = 1;
    while (i <= n) {
        result = result * i;
        i++;
    }
    return result;
}

int main() {
    int num = 5;
    int fact = factorial(num);
    return fact;
}
