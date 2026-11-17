import "ifj25" for Ifj
class Program {
    static f(a) {
        return f(a, 10)
    }
    static f(a, b) {
        return a + b
    }
    static isEven(n) {
        if (n == 0) {
            return 1
        } else {
            var m
            m = n - 1
            return isOdd(m)
        }
    }
    static isOdd(n) {
        if (n == 0) {
            return null
        } else {
            var m
            m = n - 1
            return isEven(m)
        }
    }
    static main() {
        var x
        x = f(5)
        Ifj.write(x)
        Ifj.write("\n")
        x = isEven(7)
        Ifj.write(x)
        Ifj.write("\n")
    }
}
