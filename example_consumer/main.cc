import example_module;
import std;

namespace em = example_module;

int main() {
    for (double x = -5.0; x <= 5.0; ++x) {
        std::println("{:3} {:3} {:3}", x, em::scalar(x), em::d_scalar(x));
    }

    return 0;
}
