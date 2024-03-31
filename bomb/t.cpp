#include <iostream>

int f(int x,int y){
    if (x == 0) return 0;
    if (x == 1) return y;
    return f(x-1,y) + f(x-2,y) + y;
}

int main() {
    // Example usage:
    int result = f(6, 3);
    std::cout << "Result: " << result << std::endl;

    return 0;
}
