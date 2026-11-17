import "ifj25" for Ifj
class Program {
    static counter {
        if (__count) {
        } else {
            __count = 0
        }
        __count = __count + 1
        return __count
    }
    static counter=(v) {
        __count = v
    }
    static main() {
        Ifj.write(__never)
        Ifj.write("\n")
        var a
        a = counter
        a = counter
        Ifj.write(a)
        counter = 100
        a = counter
        Ifj.write(" ")
        Ifj.write(a)
        Ifj.write("\n")
    }
}
